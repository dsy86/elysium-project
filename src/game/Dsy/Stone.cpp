#include "Stone.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "UpdateData.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseImpl.h"
#include "Database/SQLStorageImpl.h"
#include "Policies/SingletonImp.h"
#include "SQLStorages.h"
#include "Player.h"


void StoneMgr::LoadStoneLevelInfo(bool reload)
{
    m_StoneLevelInfo.clear();
    uint32 count = 0;

    //                                                0     1         2        3      4       5        6         7      8
    QueryResult *result = WorldDatabase.Query("SELECT level,req_item,req_count,health,agility,strength,intellect,spirit,stamina FROM dsy_stone_level_info");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 stone level infos. DB table `dsy_stone_level_info` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field *fields = result->Fetch();
        bar.step();

        uint32 level        = fields[0].GetUInt32();
        StoneLevelInfo levelInfo;
        levelInfo.level     = level;
        levelInfo.reqItem   = fields[1].GetUInt32();
        levelInfo.reqCount  = fields[2].GetUInt32();
        levelInfo.health    = fields[3].GetUInt32();
        levelInfo.agility   = fields[4].GetUInt32();
        levelInfo.strength  = fields[5].GetUInt32();
        levelInfo.intellect = fields[6].GetUInt32();
        levelInfo.spirit    = fields[7].GetUInt32();
        levelInfo.stamina   = fields[8].GetUInt32();
        m_StoneLevelInfo[level] = levelInfo;
        m_MaxLevel = level;
        ++count;
    } while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %lu level infos", (unsigned long)m_StoneLevelInfo.size());
}

void StoneMgr::LoadStoneGradeInfo(bool reload)
{
    m_StoneGradeInfo.clear();
    uint32 count = 0;

    //                                                0     1         2        3        4        5        6        7        8
    QueryResult *result = WorldDatabase.Query("SELECT grade,req_count,quality0,quality1,quality2,quality3,quality4,quality5,quality6 FROM dsy_stone_grade_info");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 stone grade infos. DB table `dsy_stone_grade_info` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field *fields = result->Fetch();
        bar.step();

        uint32 grade = fields[0].GetUInt32();
        StoneGradeInfo gradeInfo;
        gradeInfo.grade = grade;
        gradeInfo.reqCount = fields[1].GetUInt32();
        gradeInfo.rate[0] = fields[2].GetFloat();
        gradeInfo.rate[1] = fields[3].GetFloat();
        gradeInfo.rate[2] = fields[4].GetFloat();
        gradeInfo.rate[3] = fields[5].GetFloat();
        gradeInfo.rate[4] = fields[6].GetFloat();
        gradeInfo.rate[5] = fields[7].GetFloat();
        gradeInfo.rate[6] = fields[8].GetFloat();
        m_StoneGradeInfo[grade] = gradeInfo;
        m_MaxGrade = grade;
        ++count;
    } while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %lu grade infos", (unsigned long)m_StoneGradeInfo.size());

}

bool Stone::CanAddLevel()
{
    StoneLevelInfo levelInfo = sStoneMgr.GetStoneLevelInfo(m_level);
    if (m_level >= sStoneMgr.GetMaxLevel())
    {
        m_AddLevelReason = MAX_LEVEL_OR_GRADE;
        return false;
    }
    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER)
    {
        m_AddLevelReason = NO_REASON;
        return false;
    }
    if (!GetOwner()->HasItemCount(levelInfo.reqItem, levelInfo.reqCount))
    {
        m_AddLevelReason = REQUIRE_ITEM;
        return false;
    }
    m_AddLevelReason = SUCCEED;
    return true;
}

bool Stone::CanAddGrade()
{
    StoneGradeInfo gradeInfo = sStoneMgr.GetStoneGradeInfo(m_grade);
    if (m_grade >= sStoneMgr.GetMaxGrade())
    {
        m_AddGradeReason = MAX_LEVEL_OR_GRADE;
        return false;
    }
    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER)
    {
        m_AddGradeReason = NO_REASON;
        return false;
    }
    if (!GetOwner()->HasItemCount(GetEntry(), (gradeInfo.reqCount + 1))) //加1是为了别把本体扣了
    {
        m_AddGradeReason = REQUIRE_ITEM;
        return false;
    }
    m_AddGradeReason = SUCCEED;
    return true;
}

bool Stone::AddLevel()
{
    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER) return false;
    Player *owner = GetOwner();
    if (CanAddLevel())
    {
        StoneLevelInfo levelInfo = sStoneMgr.GetStoneLevelInfo(m_level);
        if (!owner) return false;
        owner->DestroyItemCount(levelInfo.reqItem, levelInfo.reqCount, true);
        SetLevel(m_level + 1);
        owner->SendMsgHint(_StringToUTF8("护身符升级成功"));
        return true;
    }
    switch (m_AddLevelReason)
    {
    case MAX_LEVEL_OR_GRADE:
        owner->SendErrorMsgHint(_StringToUTF8("等级已满，无法继续升级"));
        break;
    case REQUIRE_ITEM:
        owner->SendErrorMsgHint(_StringToUTF8("材料不足，无法升级"));
        break;
    case SUCCEED:
    case NO_REASON:
    default:
        break;
    }
    return false;
}

bool Stone::AddGrade()
{
    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER) return false;
    Player *owner = GetOwner();
    if (CanAddGrade())
    {
        StoneGradeInfo gradeInfo = sStoneMgr.GetStoneGradeInfo(m_grade);
        if (!owner) return false;
        owner->DestroyItemCount(GetEntry(), gradeInfo.reqCount, true);
        SetGrade(m_grade + 1);
        owner->SendMsgHint(_StringToUTF8("护身符升阶成功"));
        return true;
    }
    switch (m_AddGradeReason)
    {
    case MAX_LEVEL_OR_GRADE:
        owner->SendErrorMsgHint(_StringToUTF8("阶级已满，无法继续升阶"));
        break;
    case REQUIRE_ITEM:
        owner->SendErrorMsgHint(_StringToUTF8("材料不足，无法升阶"));
        break;
    case SUCCEED:
    case NO_REASON:
    default:
        break;
    }
    return false;
}

uint32 Stone::GetStatValue(ItemModType statType)
{
    float value = 0;
	StoneLevelInfo levelInfo = sStoneMgr.GetStoneLevelInfo(m_level);
	StoneGradeInfo gradeInfo = sStoneMgr.GetStoneGradeInfo(m_grade);
    uint32 quality = GetProto()->Quality > 6 ? 0 : GetProto()->Quality;
    switch (statType)
    {
    case ITEM_MOD_HEALTH:                           // modify HP
        value = levelInfo.health * gradeInfo.rate[quality];
        break;
    case ITEM_MOD_AGILITY:                          // modify agility
        value = levelInfo.agility * gradeInfo.rate[quality];
        break;
    case ITEM_MOD_STRENGTH:                         //modify strength
        value = levelInfo.strength * gradeInfo.rate[quality];
        break;
    case ITEM_MOD_INTELLECT:                        //modify intellect
        value = levelInfo.intellect * gradeInfo.rate[quality];
        break;
    case ITEM_MOD_SPIRIT:                           //modify spirit
        value = levelInfo.spirit * gradeInfo.rate[quality];
        break;
    case ITEM_MOD_STAMINA:                          //modify stamina
        value = levelInfo.stamina * gradeInfo.rate[quality];
        break;
    default:
        break;
    }
    return static_cast<uint32>(value);
}

void Stone::ApplyStoneStats(bool apply, Player* forplayer/* = nullptr*/)
{
    if (apply == m_StatsApplied) return; //如果已经加了属性，还要加属性的话，或者如果没加属性，还要减属性的话，让他去死
    if (!forplayer)
    {
        if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER) return;
        forplayer = GetOwner();
    }
    if (!forplayer->IsInWorld()) return;

    forplayer->HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(GetStatValue(ITEM_MOD_HEALTH)), apply);// modify HP

    forplayer->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(GetStatValue(ITEM_MOD_AGILITY)), apply); // modify agility
    forplayer->ApplyStatBuffMod(STAT_AGILITY, float(GetStatValue(ITEM_MOD_AGILITY)), apply);

    forplayer->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(GetStatValue(ITEM_MOD_STRENGTH)), apply);//modify strength
    forplayer->ApplyStatBuffMod(STAT_STRENGTH, float(GetStatValue(ITEM_MOD_STRENGTH)), apply);

    forplayer->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(GetStatValue(ITEM_MOD_INTELLECT)), apply); //modify intellect
    forplayer->ApplyStatBuffMod(STAT_INTELLECT, float(GetStatValue(ITEM_MOD_INTELLECT)), apply);

    forplayer->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(GetStatValue(ITEM_MOD_SPIRIT)), apply); //modify spirit
    forplayer->ApplyStatBuffMod(STAT_SPIRIT, float(GetStatValue(ITEM_MOD_SPIRIT)), apply);

    forplayer->HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(GetStatValue(ITEM_MOD_STAMINA)), apply); //modify stamina
    forplayer->ApplyStatBuffMod(STAT_STAMINA, float(GetStatValue(ITEM_MOD_STAMINA)), apply);

    m_StatsApplied = apply;
}


bool Stone::LoadFromDB(uint32 guidLow, ObjectGuid ownerGuid, Field* fields, uint32 entry)
{
    if (!Item::LoadFromDB(guidLow, ownerGuid, fields, entry))
        return false;

    SetLevel(fields[14].GetUInt32());
    SetGrade(fields[15].GetUInt32());
    //ApplyStoneStats(true); //在player那已经apply过了
    return true;
}

void Stone::SaveToDB()
{
    Item::SaveToDB();
}

StoneMgr& StoneMgr::Instance()
{
    static StoneMgr stoneMgr;
    return stoneMgr;
}

StoneLevelInfo StoneMgr::GetStoneLevelInfo(uint32 level)
{
    return m_StoneLevelInfo[level];
}

StoneGradeInfo StoneMgr::GetStoneGradeInfo(uint32 grade)
{
    return m_StoneGradeInfo[grade];
}