#ifndef SPELLTIMERMGR_H
#define SPELLTIMERMGR_H

#include "SpellTimer.h"
#include "Unit.h"

typedef std::map<uint32, SpellTimer*> SpellTimerMap;
typedef std::list<uint32> TimerIdList;

#define GCD_ID 61304
#define QUEUE_TIMER_ID 100000

struct SpellTimerMgr
{
    public:
        SpellTimerMgr(Unit* unit);
        ~SpellTimerMgr();

        void AddTimer(uint32 timerId, uint32 initialSpellId, RV initialTimer, RV initialCooldown, UnitSelectType targetType = UNIT_SELECT_NONE, CastType castType = CAST_TYPE_NONCAST, uint64 targetInfo = 0, Unit* caster = NULL);
        void RemoveTimer(uint32 timerId);               // not safe to use right now
        SpellTimer* GetTimer(uint32 timerId);

        void UpdateTimers(uint32 const uiDiff);
        SpellTimer* GetState(uint32 timerId, Unit* target = NULL);
        void AddToCastQueue(uint32 timerId) { m_IdToBeCasted.push_back(timerId); }
        void AddSpellToQueue(uint32 spellId, UnitSelectType targetType = UNIT_SELECT_SELF, uint64 targetInfo = 0);
        bool AddSpellCast(uint32 spellId, UnitSelectType targetType = UNIT_SELECT_SELF, uint64 targetInfo = 0);

        void Cooldown(uint32 timerId, RV changedCD = NULL, bool permanent = false);
        bool IsReady(uint32 timerId);

        void Reset(uint32 timerId, TimerValues value);
        void SetValue(uint32 timerId, TimerValues value, uint64 newValue);
        uint32 GetValue(uint32 timerId, TimerValues value);

        bool CanFinishTimer(SpellTimer* timer);
        bool CanFinishTimer(uint32 timerId) { return CanFinishTimer(GetTimer(timerId)); }
        bool FinishTimer(SpellTimer* timer, Unit* target = NULL);

        void ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs);

        void SetUpdatable(bool update) { isUpdatable = update; }
        bool IsUpdatable() const { return isUpdatable; }

        void Clear();

    private:
        SpellTimerMap m_TimerMap;
        TimerIdList m_IdToBeCasted;

        SpellTimer* m_GCD;

        Unit* m_owner;

        bool isUpdatable;
};

#endif // SPELLTIMERMGR_H
