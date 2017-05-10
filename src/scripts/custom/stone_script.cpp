/* Copyright (C) 2009 - 2010 ScriptDevZero <http://github.com/scriptdevzero/scriptdevzero>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "scriptPCH.h"
#include "custom.h"
#include "DsyMiscMgr.h"

std::vector<char*> statName = {"魔法", "生命", "", "敏捷", "力量", "智力", "精神", "耐力"};

enum Action
{
    ACTION_NONE,
    ACTION_ADDLEVEL,
    ACTION_ADDGRADE,
    ACTION_RESETLEVEL,
    ACTION_RESETGRADE
};

void ShowStatText(Player *player, Stone* stone, ItemModType statType)
{
    if (stone->GetStatValue(statType))
    {
        std::ostringstream stm;
        stm << _StringToUTF8(statName[statType]) << " + " << stone->GetStatValue(statType);
        player->ADD_GOSSIP_ITEM(5, stm.str().c_str(), GOSSIP_SENDER_MAIN, ACTION_NONE);
        stm.clear();
    }
}

std::string ReqItemText(uint32 entry, uint32 reqCount, int32 nowCount)
{
    std::string countText = "(" + std::to_string(nowCount) + "/" + std::to_string(reqCount) + ")";
    if (nowCount < (int32)reqCount)
        countText = "|cffff0000" + countText + "|r";
    std::string text = sDsyMiscMgr.GetItemLink(entry) + " " + countText;
    return text;
}

bool StoneMainMenu(Player *player, Item *item)
{
    if (!item->IsStone())
    {
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
    player->PlayerTalkClass->ClearMenus();
    Stone* stone = item->ToStone();
    player->ADD_GOSSIP_ITEM(5, std::string(_StringToUTF8("护身符等级：") + std::to_string(stone->GetLevel())).c_str(), GOSSIP_SENDER_MAIN, ACTION_NONE);
    player->ADD_GOSSIP_ITEM(5, std::string(_StringToUTF8("护身符阶级：") + std::to_string(stone->GetGrade())).c_str(), GOSSIP_SENDER_MAIN, ACTION_NONE);
    ShowStatText(player, stone, ITEM_MOD_HEALTH);
    ShowStatText(player, stone, ITEM_MOD_STRENGTH);
    ShowStatText(player, stone, ITEM_MOD_AGILITY);
    ShowStatText(player, stone, ITEM_MOD_STAMINA);
    ShowStatText(player, stone, ITEM_MOD_INTELLECT);
    ShowStatText(player, stone, ITEM_MOD_SPIRIT);
    player->ADD_GOSSIP_ITEM(5, "---------------------------", GOSSIP_SENDER_MAIN, ACTION_NONE);
    StoneLevelInfo levelInfo = sStoneMgr.GetStoneLevelInfo(stone->GetLevel());
    StoneGradeInfo gradeInfo = sStoneMgr.GetStoneGradeInfo(stone->GetGrade());
    if (stone->GetLevel() < sStoneMgr.GetMaxLevel())
    {
        player->ADD_GOSSIP_ITEM(5, std::string(_StringToUTF8("升级需要：") + ReqItemText(levelInfo.reqItem, levelInfo.reqCount, player->GetItemCount(levelInfo.reqItem))).c_str(), GOSSIP_SENDER_MAIN, ACTION_NONE);
        player->ADD_GOSSIP_ITEM(5, _StringToUTF8("     【升级】"), GOSSIP_SENDER_MAIN, ACTION_ADDLEVEL);
    }
    else
        player->ADD_GOSSIP_ITEM(5, _StringToUTF8("|cffff6060【等级已满，无法升级】|r"), GOSSIP_SENDER_MAIN, ACTION_NONE);

    if (stone->GetGrade() < sStoneMgr.GetMaxGrade())
    {
        player->ADD_GOSSIP_ITEM(5, std::string(_StringToUTF8("升阶需要：") + ReqItemText(stone->GetEntry(), gradeInfo.reqCount, (player->GetItemCount(stone->GetEntry())) - 1)).c_str(), GOSSIP_SENDER_MAIN, ACTION_NONE);
        player->ADD_GOSSIP_ITEM(5, _StringToUTF8("     【升阶】"), GOSSIP_SENDER_MAIN, ACTION_ADDGRADE);
    }
    else
        player->ADD_GOSSIP_ITEM(5, _StringToUTF8("|cffff6060【阶级已满，无法升阶】|r"), GOSSIP_SENDER_MAIN, ACTION_NONE);
    player->ADD_GOSSIP_ITEM(5, _StringToUTF8("重置等级"), GOSSIP_SENDER_MAIN, ACTION_RESETLEVEL);
    player->ADD_GOSSIP_ITEM(5, _StringToUTF8("重置阶级"), GOSSIP_SENDER_MAIN, ACTION_RESETGRADE);

    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
    return true;
}

bool StoneGossipSelect(Player *player, Item *item, uint32 sender, uint32 action)
{
    //player->SendMsgHint(_StringToUTF8("测试"));
    if (!item->IsStone())
    {
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
    player->PlayerTalkClass->ClearMenus();
    Stone* stone = item->ToStone();
    if (sender == GOSSIP_SENDER_MAIN)
    {
        switch (action)
        {
        case ACTION_ADDLEVEL:
            stone->AddLevel();
            break;
        case ACTION_ADDGRADE:
            stone->AddGrade();
            break;
        case ACTION_RESETLEVEL:
            stone->SetLevel(1);
            player->SendMsgHint(_StringToUTF8("等级已重置"));
            break;
        case ACTION_RESETGRADE:
            stone->SetGrade(1);
            player->SendMsgHint(_StringToUTF8("阶级已重置"));
            break;
        case ACTION_NONE:
        default:
            break;
        }
    }
    StoneMainMenu(player, item);
    return true;
}

bool StoneUse(Player *player, Item *item, SpellCastTargets const& target)
{
    return StoneMainMenu(player, item);
}

void AddSC_StoneScript()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "StoneScript";
    newscript->pItemUse = &StoneUse;
    newscript->pItemGossipSelect = &StoneGossipSelect;
    newscript->RegisterSelf(true);
}