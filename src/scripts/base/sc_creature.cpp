/* Copyright (C) 2006 - 2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#include "precompiled.h"
#include "Item.h"
#include "Spell.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"

// Spell summary for ScriptedAI::SelectSpell
struct TSpellSummary
{
    uint8 Targets;                                          // set of enum SelectTarget
    uint8 Effects;                                          // set of enum SelectEffect
} *SpellSummary;

ScriptedAI::ScriptedAI(Creature* pCreature) : CreatureAI(pCreature),
    m_bCombatMovement(true),
    m_uiEvadeCheckCooldown(2500),
    m_bAttackEnabled(true),
    m_TimerMgr(pCreature->CreateTimerMgr())
{}

bool ScriptedAI::IsVisible(Unit* pWho) const
{
    if (!pWho)
        return false;

    return m_creature->IsWithinDist(pWho, VISIBLE_RANGE) && pWho->isVisibleForOrDetect(m_creature, m_creature, true);
}

void ScriptedAI::MoveInLineOfSight(Unit* pWho)
{
    if (m_creature->CanInitiateAttack() && pWho->isTargetableForAttack() &&
        m_creature->IsHostileTo(pWho) && pWho->isInAccessablePlaceFor(m_creature))
    {
        if (!m_creature->canFly() && m_creature->GetDistanceZ(pWho) > CREATURE_Z_ATTACK_RANGE)
            return;

        if (m_creature->IsWithinDistInMap(pWho, m_creature->GetAttackDistance(pWho)) && m_creature->IsWithinLOSInMap(pWho))
        {
            if (!m_creature->getVictim())
            {
                pWho->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
                AttackStart(pWho);
            }
            else if (m_creature->GetMap()->IsDungeon())
            {
                pWho->SetInCombatWith(m_creature);
                m_creature->AddThreat(pWho);
            }
        }
    }
}

void ScriptedAI::AttackStart(Unit* pWho)
{
    if (!pWho || !m_bAttackEnabled)
        return;

    if (m_creature->Attack(pWho, true))
    {
        m_creature->AddThreat(pWho);
        m_creature->SetInCombatWith(pWho);
        pWho->SetInCombatWith(m_creature);

        if (IsCombatMovement())
            m_creature->GetMotionMaster()->MoveChase(pWho);
    }
}

void ScriptedAI::EnterCombat(Unit* pEnemy)
{
    if (!pEnemy)
        return;

    Aggro(pEnemy);
}

void ScriptedAI::Aggro(Unit*)
{
}

void ScriptedAI::UpdateAI(const uint32 )
{
    //Check if we have a current target
    if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        return;

    DoMeleeAttackIfReady();
}

void ScriptedAI::EnterEvadeMode()
{
    m_creature->RemoveAllAuras();
    m_creature->DeleteThreatList();
    m_creature->CombatStop(true);
    m_creature->LoadCreaturesAddon();

    if (m_creature->isAlive())
        m_creature->GetMotionMaster()->MoveTargetedHome();

    m_creature->SetLootRecipient(NULL);

    Reset();
}

void ScriptedAI::JustRespawned()
{
    Reset();
}

void ScriptedAI::DoStartMovement(Unit* pVictim, float fDistance, float fAngle)
{
    if (pVictim)
        m_creature->GetMotionMaster()->MoveChase(pVictim, fDistance, fAngle);
}

void ScriptedAI::DoStartNoMovement(Unit* pVictim)
{
    if (!pVictim)
        return;

    m_creature->GetMotionMaster()->MoveIdle();
    m_creature->StopMoving();
}

void ScriptedAI::DoMeleeAttackIfReady()
{
    if (!m_bAttackEnabled)
        return;

    if (m_creature->IsWithinDistInMap(m_creature->getVictim(), ATTACK_DISTANCE))
    {
        if (m_creature->haveOffhandWeapon() && m_creature->isAttackReady(OFF_ATTACK))
        {
            if(m_creature->isAttackReady())
                m_creature->setAttackTimer(OFF_ATTACK, m_creature->GetCreatureInfo()->offattacktime/2);
            else
            { 
                m_creature->AttackerStateUpdate(m_creature->getVictim(), OFF_ATTACK);
                m_creature->resetAttackTimer(OFF_ATTACK);
            }
        }

        if (m_creature->isAttackReady())
        {
            m_creature->AttackerStateUpdate(m_creature->getVictim());
            m_creature->resetAttackTimer();
        }
    }
}

void ScriptedAI::DoStopAttack()
{
    if (m_creature->getVictim())
        m_creature->AttackStop();
}

void ScriptedAI::DoCast(Unit* pTarget, uint32 uiSpellId, bool bTriggered)
{
    if (m_creature->IsNonMeleeSpellCasted(false) && !bTriggered)
        return;

    m_creature->CastSpell(pTarget, uiSpellId, bTriggered);
}

void ScriptedAI::DoCastSpell(Unit* pTarget, SpellEntry const* pSpellInfo, bool bTriggered)
{
    if (m_creature->IsNonMeleeSpellCasted(false) && !bTriggered)
        return;

    m_creature->CastSpell(pTarget, pSpellInfo, bTriggered);
}

void ScriptedAI::DoPlaySoundToSet(WorldObject* pSource, uint32 uiSoundId)
{
    if (!pSource)
        return;

    if (!GetSoundEntriesStore()->LookupEntry(uiSoundId))
    {
        error_log("SD2: Invalid soundId %u used in DoPlaySoundToSet (Source: TypeId %u, GUID %u)", uiSoundId, pSource->GetTypeId(), pSource->GetGUIDLow());
        return;
    }

    pSource->PlayDirectSound(uiSoundId);
}

Creature* ScriptedAI::DoSpawnCreature(uint32 uiId, float fX, float fY, float fZ, float fAngle, uint32 uiType, uint32 uiDespawntime, bool update_z)
{
    return m_creature->SummonCreature(uiId,m_creature->GetPositionX()+fX, m_creature->GetPositionY()+fY, m_creature->GetPositionZ()+fZ, fAngle, (TempSummonType)uiType, uiDespawntime, update_z);
}

SpellEntry const* ScriptedAI::SelectSpell(Unit* pTarget, int32 uiSchool, int32 uiMechanic, SelectTarget selectTargets, uint32 uiPowerCostMin, uint32 uiPowerCostMax, float fRangeMin, float fRangeMax, SelectEffect selectEffects)
{
    //No target so we can't cast
    if (!pTarget)
        return false;

    //Silenced so we can't cast
    if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
        return false;

    //Using the extended script system we first create a list of viable spells
    SpellEntry const* apSpell[4];
    memset(apSpell, 0, sizeof(SpellEntry*)*4);

    uint32 uiSpellCount = 0;

    SpellEntry const* pTempSpell;
    SpellRangeEntry const* pTempRange;

    //Check if each spell is viable(set it to null if not)
    for (uint32 i = 0; i < 4; ++i)
    {
        pTempSpell = GetSpellStore()->LookupEntry(m_creature->m_spells[i]);

        //This spell doesn't exist
        if (!pTempSpell)
            continue;

        // Targets and Effects checked first as most used restrictions
        //Check the spell targets if specified
        if (selectTargets && !(SpellSummary[m_creature->m_spells[i]].Targets & (1 << (selectTargets-1))))
            continue;

        //Check the type of spell if we are looking for a specific spell type
        if (selectEffects && !(SpellSummary[m_creature->m_spells[i]].Effects & (1 << (selectEffects-1))))
            continue;

        //Check for school if specified
        if (uiSchool >= 0 && pTempSpell->SchoolMask & uiSchool)
            continue;

        //Check for spell mechanic if specified
        if (uiMechanic >= 0 && pTempSpell->Mechanic != uiMechanic)
            continue;

        //Make sure that the spell uses the requested amount of power
        if (uiPowerCostMin &&  pTempSpell->manaCost < uiPowerCostMin)
            continue;

        if (uiPowerCostMax && pTempSpell->manaCost > uiPowerCostMax)
            continue;

        //Continue if we don't have the mana to actually cast this spell
        if (pTempSpell->manaCost > m_creature->GetPower((Powers)pTempSpell->powerType))
            continue;

        //Get the Range
        pTempRange = GetSpellRangeStore()->LookupEntry(pTempSpell->rangeIndex);

        //Spell has invalid range store so we can't use it
        if (!pTempRange)
            continue;

        //Check if the spell meets our range requirements
        if (fRangeMin && pTempRange->maxRange < fRangeMin)
            continue;

        if (fRangeMax && pTempRange->maxRange > fRangeMax)
            continue;

        //Check if our target is in range
        if (m_creature->IsWithinDistInMap(pTarget, pTempRange->minRange) || !m_creature->IsWithinDistInMap(pTarget, pTempRange->maxRange))
            continue;

        //All good so lets add it to the spell list
        apSpell[uiSpellCount] = pTempSpell;
        ++uiSpellCount;
    }

    //We got our usable spells so now lets randomly pick one
    if (!uiSpellCount)
        return NULL;

    return apSpell[rand()%uiSpellCount];
}

bool ScriptedAI::CanCast(Unit* pTarget, SpellEntry const* pSpellEntry, bool bTriggered)
{
    //No target so we can't cast
    if (!pTarget || !pSpellEntry)
        return false;

    //Silenced so we can't cast
    if (!bTriggered && m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
        return false;

    //Check for power
    if (!bTriggered && m_creature->GetPower((Powers)pSpellEntry->powerType) < pSpellEntry->manaCost)
        return false;

    SpellRangeEntry const* pTempRange = GetSpellRangeStore()->LookupEntry(pSpellEntry->rangeIndex);

    //Spell has invalid range store so we can't use it
    if (!pTempRange)
        return false;

    //Unit is out of range of this spell
    if (!m_creature->IsInRange(pTarget, pTempRange->minRange, pTempRange->maxRange))
        return false;

    return true;
}

void FillSpellSummary()
{
    SpellSummary = new TSpellSummary[GetSpellStore()->GetNumRows()];

    SpellEntry const* pTempSpell;

    for (uint32 i=0; i < GetSpellStore()->GetNumRows(); ++i)
    {
        SpellSummary[i].Effects = 0;
        SpellSummary[i].Targets = 0;

        pTempSpell = GetSpellStore()->LookupEntry(i);
        //This spell doesn't exist
        if (!pTempSpell)
            continue;

        for (int j=0; j<3; ++j)
        {
            //Spell targets self
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SELF-1);

            //Spell targets a single enemy
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_CHAIN_DAMAGE ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CURRENT_ENEMY_COORDINATES)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_ENEMY-1);

            //Spell targets AoE at enemy
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_ENEMY-1);

            //Spell targets an enemy
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_CHAIN_DAMAGE ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CURRENT_ENEMY_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_ENEMY-1);

            //Spell targets a single friend(or self)
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_FRIEND ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_PARTY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_FRIEND-1);

            //Spell targets aoe friends
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_FRIEND-1);

            //Spell targets any friend(or self)
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_FRIEND ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_FRIEND-1);

            //Make sure that this spell includes a damage effect
            if (pTempSpell->Effect[j] == SPELL_EFFECT_SCHOOL_DAMAGE ||
                pTempSpell->Effect[j] == SPELL_EFFECT_INSTAKILL ||
                pTempSpell->Effect[j] == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEALTH_LEECH)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_DAMAGE-1);

            //Make sure that this spell includes a healing effect (or an apply aura with a periodic heal)
            if (pTempSpell->Effect[j] == SPELL_EFFECT_HEAL ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEAL_MECHANICAL ||
                (pTempSpell->Effect[j] == SPELL_EFFECT_APPLY_AURA  && pTempSpell->EffectApplyAuraName[j]== 8))
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_HEALING-1);

            //Make sure that this spell applies an aura
            if (pTempSpell->Effect[j] == SPELL_EFFECT_APPLY_AURA)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_AURA-1);
        }
    }
}

void ScriptedAI::DoResetThreat()
{
    if (!m_creature->CanHaveThreatList() || m_creature->getThreatManager().isThreatListEmpty())
    {
        error_log("SD2: DoResetThreat called for creature that either cannot have threat list or has empty threat list (m_creature entry = %d)", m_creature->GetEntry());
        return;
    }

    ThreatList const& tList = m_creature->getThreatManager().getThreatList();
    for (ThreatList::const_iterator itr = tList.begin();itr != tList.end(); ++itr)
    {
        Unit* pUnit = Unit::GetUnit((*m_creature), (*itr)->getUnitGuid());

        if (pUnit && m_creature->getThreatManager().getThreat(pUnit))
            m_creature->getThreatManager().modifyThreatPercent(pUnit, -100);
    }
}

void ScriptedAI::DoTeleportPlayer(Unit* pUnit, float fX, float fY, float fZ, float fO)
{
    if (!pUnit || pUnit->GetTypeId() != TYPEID_PLAYER)
    {
        if (pUnit)
            error_log("SD2: Creature " UI64FMTD " (Entry: %u) Tried to teleport non-player unit (Type: %u GUID: " UI64FMTD ") to x: %f y:%f z: %f o: %f. Aborted.", m_creature->GetGUID(), m_creature->GetEntry(), pUnit->GetTypeId(), pUnit->GetGUID(), fX, fY, fZ, fO);

        return;
    }

    ((Player*)pUnit)->TeleportTo(pUnit->GetMapId(), fX, fY, fZ, fO, TELE_TO_NOT_LEAVE_COMBAT);
}

Unit* ScriptedAI::DoSelectLowestHpFriendly(float fRange, uint32 uiMinHPDiff)
{
    Unit* pUnit = NULL;

    MaNGOS::MostHPMissingInRange u_check(m_creature, fRange, uiMinHPDiff);
    MaNGOS::UnitLastSearcher<MaNGOS::MostHPMissingInRange> searcher(m_creature, pUnit, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pUnit;
}

CreatureList ScriptedAI::DoFindFriendlyCC(float fRange)
{
    CreatureList pList;

    MaNGOS::FriendlyCCedInRange u_check(m_creature, fRange);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyCCedInRange> searcher(m_creature, pList, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pList;
}

CreatureList ScriptedAI::DoFindFriendlyMissingBuff(float fRange, uint32 uiSpellId)
{
    CreatureList pList;

    MaNGOS::FriendlyMissingBuffInRange u_check(m_creature, fRange, uiSpellId);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyMissingBuffInRange> searcher(m_creature, pList, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pList;
}

Player* ScriptedAI::GetPlayerAtMinimumRange(float fMinimumRange)
{
    Player* pPlayer = NULL;

    MaNGOS::AnyPlayerInObjectRangeCheck check(m_creature, fMinimumRange);
    MaNGOS::PlayerSearcher<MaNGOS::AnyPlayerInObjectRangeCheck> searcher(m_creature, pPlayer, check);

    Cell::VisitWorldObjects(m_creature, searcher, fMinimumRange);

    return pPlayer;
}

void ScriptedAI::SetEquipmentSlots(bool bLoadDefault, int32 uiMainHand, int32 uiOffHand, int32 uiRanged)
{
    if (bLoadDefault)
    {
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->equipmentId,true);
        return;
    }

    if (uiMainHand >= 0)
        m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, uint32(uiMainHand));

    if (uiOffHand >= 0)
        m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, uint32(uiOffHand));

    if (uiRanged >= 0)
        m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, uint32(uiRanged));
}

void ScriptedAI::SetCombatMovement(bool bCombatMove)
{
    m_bCombatMovement = bCombatMove;
}

// Hacklike storage used for misc creatures that are expected to evade of outside of a certain area.
// It is assumed the information is found elswehere and can be handled by mangos. So far no luck finding such information/way to extract it.
enum
{
    NPC_BROODLORD   = 12017,
    NPC_VOID_REAVER = 19516,
    NPC_JAN_ALAI    = 23578,
    NPC_SARTHARION  = 28860
};

bool ScriptedAI::EnterEvadeIfOutOfCombatArea(const uint32 uiDiff)
{
    if (m_uiEvadeCheckCooldown < uiDiff)
        m_uiEvadeCheckCooldown = 2500;
    else
    {
        m_uiEvadeCheckCooldown -= uiDiff;
        return false;
    }

    if (m_creature->IsInEvadeMode() || !m_creature->getVictim())
        return false;

    float fX = m_creature->GetPositionX();
    float fY = m_creature->GetPositionY();
    float fZ = m_creature->GetPositionZ();

    switch(m_creature->GetEntry())
    {
        case NPC_BROODLORD:                                 // broodlord (not move down stairs)
            if (fZ > 448.60f)
                return false;
            break;
        case NPC_VOID_REAVER:                               // void reaver (calculate from center of room)
            if (m_creature->GetDistance2d(432.59f, 371.93f) < 105.0f)
                return false;
            break;
        case NPC_JAN_ALAI:                                  // jan'alai (calculate by Z)
            if (fZ > 12.0f)
                return false;
            break;
        case NPC_SARTHARION:                                // sartharion (calculate box)
            if (fX > 3218.86f && fX < 3275.69f && fY < 572.40f && fY > 484.68f)
                return false;
            break;
        default:
            error_log("SD2: EnterEvadeIfOutOfCombatArea used for creature entry %u, but does not have any definition.", m_creature->GetEntry());
            return false;
    }

    EnterEvadeMode();
    return true;
}

bool ScriptedAI::HandleTimer(uint32 &timer, const uint32 diff, bool force)
{
    if (timer < diff)
    {
        if (!force && m_creature->IsNonMeleeSpellCasted(false))
            return false;

        return true;

    }
    else
    {
        timer -= diff;
        return false;
    }
}

void Scripted_NoMovementAI::AttackStart(Unit* pWho)
{
    if (!pWho)
        return;

    if (m_creature->Attack(pWho, true))
    {
        m_creature->AddThreat(pWho);
        m_creature->SetInCombatWith(pWho);
        pWho->SetInCombatWith(m_creature);

        DoStartNoMovement(pWho);
    }
}

PlrList ScriptedAI::GetAttackingPlayers(bool not_select_current_victim)
{
    PlrList pList;
    ThreatList const& t_list = m_creature->getThreatManager().getPlayerThreatList();
    for(ThreatList::const_iterator itr = t_list.begin(); itr != t_list.end(); ++itr)
        if (Player* plr = sObjectMgr.GetPlayer((*itr)->getUnitGuid()))
            if (plr->IsInWorld() && plr->isAlive() && (plr != m_creature->getVictim() || !not_select_current_victim))
                pList.push_back(plr);

    return pList;
}

PlrList ScriptedAI::GetRandomPlayers(uint8 count, bool not_select_current_victim)
{
    PlrList pList = GetAttackingPlayers(not_select_current_victim);

    uint8 erase_count = (pList.size() > count) ? (pList.size() - count) : 0;
    for(uint8 i = 0; i < erase_count; ++i)
    {
        PlrList::iterator itr = pList.begin();
        std::advance(itr, urand(0, pList.size()-1));
        pList.erase(itr);
    }

    return pList;
}

PlrList ScriptedAI::GetRandomPlayersInRange(uint8 count, uint8 min_count, float min_range, float max_range, bool not_select_current_victim)
{
    // fill list of all player
    PlrList fullList = GetAttackingPlayers(not_select_current_victim);
    if (fullList.empty())
        return fullList;
    
    PlrList inRangeList;
    // fill list of players in range
    for(PlrList::iterator itr = fullList.begin(); itr != fullList.end(); ++itr)
        if (m_creature->IsInRange(m_creature, min_range, max_range))
            inRangeList.push_back(*itr);

    PlrList& finalList = (inRangeList.size() < min_count) ? fullList : inRangeList;
    uint8 erase_count = (finalList.size() > count) ? (finalList.size() - count) : 0;
    for(uint8 i = 0; i < erase_count; ++i)
    {
        PlrList::iterator itr = finalList.begin();
        std::advance(itr, urand(0, finalList.size()-1));
        finalList.erase(itr);
    }

    return finalList;
}

Player* ScriptedAI::SelectRandomPlayerInRange(uint8 min_ranged_count, float min_range, float max_range, bool not_select_current_victim)
{
    PlrList inRangeList = GetRandomPlayersInRange(1, min_ranged_count, min_range, max_range, not_select_current_victim);
    if (!inRangeList.empty())
        return inRangeList.front();

    return NULL;
}

void ScriptedAI::DespawnAllWithEntry(uint32 entry, TypeID type)
{
    if (!type || type == TYPEID_UNIT)
    {
        CreatureList list;
        GetCreatureListWithEntryInGrid(list, m_creature, entry, DEFAULT_VISIBILITY_INSTANCE);
        for(CreatureList::iterator itr = list.begin(); itr != list.end(); ++itr)
            (*itr)->ForcedDespawn();
    }
    if (!type || type == TYPEID_GAMEOBJECT)
    {
        GameObjectList list;
        GetGameObjectListWithEntryInGrid(list, m_creature, entry, DEFAULT_VISIBILITY_INSTANCE);
        for(GameObjectList::iterator itr = list.begin(); itr != list.end(); ++itr)
            (*itr)->Delete();
    }
}
void ScriptedAI::AddTimer(uint32 timerId, uint32 initialSpellId, RV initialTimer, RV initialCooldown, UnitSelectType targetType, CastType castType, uint64 targetInfo, Unit *caster)
{
    if (SpellTimer* timer = m_TimerMgr->GetTimer(timerId))
        timer->Reset(TIMER_VALUE_ALL);
    else
        m_TimerMgr->AddTimer(timerId, initialSpellId, initialTimer, initialCooldown, targetType, castType, targetInfo, caster);
}

void ScriptedAI::AddNonCastTimer(uint32 timerId, RV initialTimer, RV cooldown)
{
    if (SpellTimer* timer = m_TimerMgr->GetTimer(timerId))
        timer->Reset(TIMER_VALUE_ALL);
    else
        m_TimerMgr->AddTimer(timerId, 0, initialTimer, cooldown, UNIT_SELECT_NONE, CAST_TYPE_IGNORE);
}
