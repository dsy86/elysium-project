#ifndef MANGOSSERVER_DSYMISCMGR_H
#define MANGOSSERVER_DSYMISCMGR_H

#include "Common.h"
#include "ItemPrototype.h"
#include "Item.h"
#include "SharedDefines.h"
#include "DBCEnums.h"
#include "Group.h"
#include "Player.h"
#include "Weather.h"
#include "World.h"


class DsyMiscMgr
{
public:
    static DsyMiscMgr& Instance();
    std::string GetItemLink(uint32 entry);
    std::string ShowImage(std::string name, uint32 width = 24, uint32 height = 24, uint32 x = 0, uint32 y = 0) const;
    std::string goldIcon = ShowImage("MONEYFRAME/UI-GoldIcon", 12, 12);
    std::string FFButton = ShowImage("TIMEMANAGER/FFButton");
    std::string ResetButton = ShowImage("TIMEMANAGER/ResetButton");
    std::string nextButton = ShowImage("BUTTONS/UI-SpellbookIcon-NextPage-Up");
    std::string prevButton = ShowImage("BUTTONS/UI-SpellbookIcon-PrevPage-Up");
    std::string ui_raidframe_arrow = ShowImage("RAIDFRAME/UI-RAIDFRAME-ARROW");
    std::string addButton = ShowImage("GuildBankFrame/UI-GuildBankFrame-NewTab");
    std::string addButton2 = ShowImage("PaperDollInfoFrame/Character-Plus", 12, 12);
    std::string blizzard = ShowImage("CHATFRAME/UI-CHATICON-BLIZZ", 24, 12);
};

#define sDsyMiscMgr DsyMiscMgr::Instance()

#endif