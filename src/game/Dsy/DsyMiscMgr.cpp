#include "DsyMiscMgr.h"

DsyMiscMgr& DsyMiscMgr::Instance()
{
    static DsyMiscMgr dsyMiscMgr;
    return dsyMiscMgr;
}

std::string DsyMiscMgr::GetItemLink(uint32 entry)
{
    ItemPrototype const* pItem = ObjectMgr::GetItemPrototype(entry);
    if (!pItem)
        return "";
    std::string name = pItem->Name1;
    if (ItemLocale const *il = sObjectMgr.GetItemLocale(entry))
        ObjectMgr::GetLocaleString(il->Name, LOCALE_zhCN, name);

    std::ostringstream oss;
    oss << "|c" << std::hex << ItemQualityColors[pItem->Quality] << std::dec <<
        "|Hitem:" << entry << ":0:" << "0:0:0:0|h[" << name << "]|h|r";
    return oss.str();
}

void Player::SendMsgHint(std::string msg, bool posstive/* = true*/)
{
    if (msg.length() > 0)
    {
        if (posstive)
            msg = "|cffffff00" + msg + "|r";
        else
            msg = "|cffff0000" + msg + "|r";
        GetSession()->SendAreaTriggerMessage("%s", msg.c_str());
        ChatHandler(GetSession()).SendSysMessage(msg.c_str());
    }
}
void Player::SendErrorMsgHint(std::string msg)
{
    SendMsgHint(msg, false);
}

std::string DsyMiscMgr::ShowImage(std::string name, uint32 width/* = 24*/, uint32 height/* = 24*/, uint32 x/* = 0*/, uint32 y/* = 0*/) const
{
    std::string str = "|TInterface/" + name + ":" + std::to_string(height) + ":" + std::to_string(width) + ":" + std::to_string(x) + ":" + std::to_string(y) + "|t";
    return str;
}

void WorldSession::SendListInventory(uint32 creatureEntry, uint32 currencyItemEntry)
{
    DEBUG_LOG("WORLD: Sent SMSG_LIST_INVENTORY");
    SetCurrentVendorEntry(creatureEntry);
    SetCurrentCurrencyItemEntry(currencyItemEntry);
    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    VendorItemData const* vItems = sObjectMgr.GetNpcVendorItemList(creatureEntry);
    VendorItemData const* tItems = nullptr;
    uint32 vendorId = sObjectMgr.GetCreatureTemplate(creatureEntry)->vendorId;
    if (vendorId)
        tItems = sObjectMgr.GetNpcVendorTemplateItemList(vendorId);

    if (!vItems && !tItems)
    {
        WorldPacket data(SMSG_LIST_INVENTORY, (8 + 1 + 1));
        data << ObjectGuid(GetPlayer()->GetGUID());
        data << uint8(0);                                   // count==0, next will be error code
        data << uint8(0);                                   // "Vendor has no inventory"
        SendPacket(&data);
        return;
    }

    uint8 customitems = vItems ? vItems->GetItemCount() : 0;
    uint8 numitems = customitems + (tItems ? tItems->GetItemCount() : 0);

    uint8 count = 0;

    WorldPacket data(SMSG_LIST_INVENTORY, (8 + 1 + numitems * 7 * 4));
    data << ObjectGuid(GetPlayer()->GetGUID());

    size_t count_pos = data.wpos();
    data << uint8(count);

    for (int i = 0; i < numitems; ++i)
    {
        VendorItem const* crItem = i < customitems ? vItems->GetItem(i) : tItems->GetItem(i - customitems);

        if (crItem)
        {
            if (ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(crItem->item))
            {
                if (!_player->isGameMaster())
                {
                    // class wrong item skip only for bindable case
                    if ((pProto->AllowableClass & _player->getClassMask()) == 0 && pProto->Bonding == BIND_WHEN_PICKED_UP)
                        continue;

                    // race wrong item skip always
                    if ((pProto->AllowableRace & _player->getRaceMask()) == 0)
                        continue;
                }

                ++count;

                // reputation discount
                uint32 price = uint32(floor(pProto->BuyPrice));

                data << uint32(count);
                data << uint32(crItem->item);
                data << uint32(pProto->DisplayInfoID);
                data << uint32(crItem->maxcount <= 0 ? 0xFFFFFFFF : crItem->maxcount);
                data << uint32(price);
                data << uint32(pProto->MaxDurability);
                data << uint32(pProto->BuyCount);
            }
        }
    }

    if (count == 0)
    {
        data << uint8(0);                                   // "Vendor has no inventory"
        SendPacket(&data);
        return;
    }

    data.put<uint8>(count_pos, count);
    SendPacket(&data);
}

bool Player::BuySpecialItem(uint32 item, uint8 count, uint32 creatureEntry, uint32 currencyItemEntry)
{
    // cheating attempt
    if (count < 1) count = 1;

    if (!isAlive())
        return false;

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(item);
    if (!pProto)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, item, 0);
        return false;
    }

    VendorItemData const* vItems = sObjectMgr.GetNpcVendorItemList(creatureEntry);
    VendorItemData const* tItems = nullptr;
    uint32 vendorId = sObjectMgr.GetCreatureTemplate(creatureEntry)->vendorId;
    if (vendorId)
        tItems = sObjectMgr.GetNpcVendorTemplateItemList(vendorId);

    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, nullptr, item, 0);
        return false;
    }

    uint32 vCount = vItems ? vItems->GetItemCount() : 0;
    uint32 tCount = tItems ? tItems->GetItemCount() : 0;

    size_t vendorslot = vItems ? vItems->FindItemSlot(item) : vCount;
    if (vendorslot > vCount)
        vendorslot = vCount + (tItems ? tItems->FindItemSlot(item) : tCount);

    if (vendorslot >= vCount + tCount)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, nullptr, item, 0);
        return false;
    }

    VendorItem const* crItem = vendorslot < vCount ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - vCount);
    if (!crItem || crItem->item != item)                    // store diff item (cheating)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, nullptr, item, 0);
        return false;
    }

    uint32 totalCount = pProto->BuyCount * count;

    // not check level requiremnt for normal items (PvP related bonus items is another case)
    if (pProto->RequiredHonorRank && (m_honorMgr.GetHighestRank().rank < (uint8)pProto->RequiredHonorRank || getLevel() < pProto->RequiredLevel))
    {
        SendBuyError(BUY_ERR_RANK_REQUIRE, nullptr, item, 0);
        return false;
    }

    uint32 price = floor(pProto->BuyPrice / 10000) * count;

    if (!HasItemCount(currencyItemEntry, price))
    {
        SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, nullptr, item, 0);
        return false;
    }

    Item* pItem = NULL;

    ItemPosCountVec dest;
    InventoryResult msg = CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item, totalCount);
    if (msg != EQUIP_ERR_OK)
    {
        SendEquipError(msg, NULL, NULL, item);
        return false;
    }

    //ModifyMoney(-int32(price));
    DestroyItemCount(currencyItemEntry, price, true);

    pItem = StoreNewItem(dest, item, true);

    if (!pItem)
        return false;

    WorldPacket data(SMSG_BUY_ITEM, 8 + 4 + 4 + 4);
    data << GetGUID();
    data << uint32(vendorslot + 1);                 // numbered from 1 at client
    data << uint32(0xFFFFFFFF);
    data << uint32(count);
    GetSession()->SendPacket(&data);

    SendNewItem(pItem, totalCount, true, false, false);

    return crItem->maxcount != 0;
}
