#ifndef MANGOSSERVER_STONE_H
#define MANGOSSERVER_STONE_H

#include "Common.h"
#include "ItemPrototype.h"
#include "Item.h"
#include "DsyMiscMgr.h"


struct StoneLevelInfo
{
    uint32 level;
    uint32 reqItem;
    uint32 reqCount;
    uint32 health;
    uint32 agility;
    uint32 strength;
    uint32 intellect;
    uint32 spirit;
    uint32 stamina;
};
typedef std::map<uint32, StoneLevelInfo> StoneLevelInfoMap;

struct StoneGradeInfo
{
    uint32 grade;
    uint32 reqCount;
    float rate[7];
};
typedef std::map<uint32, StoneGradeInfo> StoneGradeInfoMap;

enum ErrorReason
{
    SUCCEED,
    MAX_LEVEL_OR_GRADE,
    REQUIRE_ITEM,
    NO_REASON
};

class Stone : public Item
{
public:
    Stone() : m_level(1), m_grade(1), m_StatsApplied(false) {} // 初始1级1阶
    bool AddLevel();
    bool AddGrade();
    void SetLevel(uint32 level) 
    { 
        if (m_level == level) return;
        ApplyStoneStats(false); //先干掉旧等级的属性
        m_level = level;
        ApplyStoneStats(true); //刷新等级的属性
        SetState(ITEM_CHANGED);
    }
    void SetGrade(uint32 grade) 
    { 
        if (m_grade == grade) return;
        ApplyStoneStats(false); //先干掉旧等级的属性
        m_grade = grade;
        ApplyStoneStats(true); //刷新等级的属性
        SetState(ITEM_CHANGED);
    }
    const uint32 GetLevel() const { return m_level; }
    const uint32 GetGrade() const { return m_grade; }
    bool CanAddLevel();
    bool CanAddGrade();
    uint32 GetStatValue(ItemModType statType);
    void ApplyStoneStats(bool apply, Player* forplayer = nullptr);
    bool GetStatApplyStatus() const { return m_StatsApplied; }

    // DB
    // overwrite virtual Item::SaveToDB
    virtual void SaveToDB();
    // overwrite virtual Item::LoadFromDB
    virtual bool LoadFromDB(uint32 guidLow, ObjectGuid ownerGuid, Field* fields, uint32 entry);
    // overwrite virtual Item::DeleteFromDB
    
protected:
    uint32 m_level;
    uint32 m_grade;
    bool m_StatsApplied;
    ErrorReason m_AddLevelReason;
    ErrorReason m_AddGradeReason;
};

class StoneMgr
{
public:
    static StoneMgr& Instance();
    void LoadStoneLevelInfo(bool reload);
    void LoadStoneGradeInfo(bool reload);
    StoneLevelInfo GetStoneLevelInfo(uint32 level);
    StoneGradeInfo GetStoneGradeInfo(uint32 grade);
    uint32 GetMaxLevel() const { return m_MaxLevel; }
    uint32 GetMaxGrade() const { return m_MaxGrade; }

private:
    StoneLevelInfoMap m_StoneLevelInfo;
    StoneGradeInfoMap m_StoneGradeInfo;
    uint32 m_MaxLevel;
    uint32 m_MaxGrade;
};

#define sStoneMgr StoneMgr::Instance()

#endif