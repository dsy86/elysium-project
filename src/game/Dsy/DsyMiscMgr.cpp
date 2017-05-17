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

std::string DsyMiscMgr::GetItemName(uint32 entry)
{
    ItemPrototype const* pItem = ObjectMgr::GetItemPrototype(entry);
    if (!pItem)
        return "";
    std::string name = pItem->Name1;
    if (ItemLocale const *il = sObjectMgr.GetItemLocale(entry))
        ObjectMgr::GetLocaleString(il->Name, LOCALE_zhCN, name);
    return "[" + name + "]";
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
    GetPlayer()->SendMsgHint(_StringToUTF8("此商店使用") + sDsyMiscMgr.GetItemLink(currencyItemEntry) + _StringToUTF8("消费，1金币=1个") + sDsyMiscMgr.GetItemLink(currencyItemEntry));
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

// copy from bool Player::BuyItemFromVendor(ObjectGuid vendorGuid, uint32 item, uint8 count, uint8 bag, uint8 slot)
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

    uint32 price = (uint32)floor(pProto->BuyPrice / 10000);
    if (price < 1) price = 1;
    price *= count;

    if (!HasItemCount(currencyItemEntry, price))
    {
        SendErrorMsgHint(_StringToUTF8("购买需要") + sDsyMiscMgr.GetItemName(currencyItemEntry) + " * " + std::to_string(price) + _StringToUTF8("，你的") + sDsyMiscMgr.GetItemName(currencyItemEntry) + _StringToUTF8("不够"));
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


//copy from void WorldSession::HandleBattlemasterJoinOpcode(WorldPacket & recv_data)
void Player::JoinBattleGround(BattleGroundTypeId bgTypeId, bool joinAsGroup/* = false*/, uint32 instanceId/* = 0*/)
{
    ObjectGuid guid = GetObjectGuid();
    bool queuedAtBGPortal = false;
    bool isPremade = false;
    Group * grp;

    queuedAtBGPortal = true;

    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        sLog.outError("Battleground: invalid bgtype (%u) received. possible cheater? player guid %u", bgTypeId, GetGUIDLow());
        return;
    }
    if (bgTypeId == BATTLEGROUND_AV && joinAsGroup)
    {
        GetSession()->ProcessAnticheatAction("SAC", "Attempt to queue AV as group.", CHEAT_ACTION_LOG);
        return;
    }

    DEBUG_LOG("WORLD: Recvd CMSG_BATTLEMASTER_JOIN Message from %s", guid.GetString().c_str());

    // can do this, since it's battleground, not arena
    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(bgTypeId);

    // ignore if player is already in BG
    if (InBattleGround())
        return;
    /*
    Creature *unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
    return;

    if (!unit->isBattleMaster())                            // it's not battlemaster
    return;
    */
    // get bg instance or bg template if instance not found
    BattleGround *bg = NULL;
    if (instanceId)
        bg = sBattleGroundMgr.GetBattleGroundThroughClientInstance(instanceId, bgTypeId);

    if (!bg && !(bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId)))
    {
        sLog.outError("Battleground: no available bg / template found");
        return;
    }

    BattleGroundBracketId bgBracketId = GetBattleGroundBracketIdFromLevel(bgTypeId);

    // check queue conditions
    if (!joinAsGroup)
    {
        // check Deserter debuff
        if (!CanJoinToBattleground())
        {
            WorldPacket data(SMSG_GROUP_JOINED_BATTLEGROUND, 4);
            data << uint32(0xFFFFFFFE);
            GetSession()->SendPacket(&data);
            return;
        }
        // check if already in queue
        if (GetBattleGroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
            //player is already in this queue
            return;
        // check if has free queue slots
        if (!HasFreeBattleGroundQueueId())
            return;
    }
    else
    {
        grp = GetGroup();
        // no group found, error
        if (!grp)
            return;
        uint32 err = grp->CanJoinBattleGroundQueue(bgTypeId, bgQueueTypeId, 0, bg->GetMaxPlayersPerTeam());
        isPremade = sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH) &&
            (grp->GetMembersCount() >= sWorld.getConfig(CONFIG_UINT32_BATTLEGROUND_PREMADE_QUEUE_GROUP_MIN_SIZE));
        if (err != BG_JOIN_ERR_OK)
        {
            GetSession()->SendBattleGroundJoinError(err);
            return;
        }
    }
    // if we're here, then the conditions to join a bg are met. We can proceed in joining.

    // GetGroup() was already checked, grp is already initialized
    BattleGroundQueue& bgQueue = sBattleGroundMgr.m_BattleGroundQueues[bgQueueTypeId];
    if (joinAsGroup)
    {
        DEBUG_LOG("Battleground: the following players are joining as group:");
        GroupQueueInfo * ginfo = bgQueue.AddGroup(this, grp, bgTypeId, bgBracketId, isPremade);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, GetBattleGroundBracketIdFromLevel(bgTypeId));
        for (GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player *member = itr->getSource();
            if (!member) continue;  // this should never happen

            uint32 queueSlot = member->AddBattleGroundQueueId(bgQueueTypeId);           // add to queue

                                                                                        // store entry point coords
            member->SetBattleGroundEntryPoint(this, queuedAtBGPortal);

            WorldPacket data;
            // send status packet (in queue)
            sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0);
            member->GetSession()->SendPacket(&data);
            DEBUG_LOG("Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, member->GetGUIDLow(), member->GetName());
        }
        DEBUG_LOG("Battleground: group end");
    }
    else
    {
        GroupQueueInfo * ginfo = bgQueue.AddGroup(this, NULL, bgTypeId, bgBracketId, isPremade);
        uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, GetBattleGroundBracketIdFromLevel(bgTypeId));
        // already checked if queueSlot is valid, now just get it
        uint32 queueSlot = AddBattleGroundQueueId(bgQueueTypeId);
        // store entry point coords
        SetBattleGroundEntryPoint(this, queuedAtBGPortal);

        WorldPacket data;
        // send status packet (in queue)
        sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0);
        GetSession()->SendPacket(&data);
        DEBUG_LOG("Battleground: player joined queue for bg queue type %u bg type %u: GUID %u, NAME %s", bgQueueTypeId, bgTypeId, GetGUIDLow(), GetName());
    }
    sBattleGroundMgr.ScheduleQueueUpdate(bgQueueTypeId, bgTypeId, GetBattleGroundBracketIdFromLevel(bgTypeId));
}

void Player::JoinBattleGround(uint32 mapId, bool joinAsGroup/* = false*/, uint32 instanceId/* = 0*/)
{
    BattleGroundTypeId bgTypeId = GetBattleGroundTypeIdByMapId(mapId);
    JoinBattleGround(bgTypeId, joinAsGroup, instanceId);
}
