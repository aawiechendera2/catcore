/* Copyright (C) 2006 - 2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: boss_algalon
SD%Complete: 0%
SDComment:
SDCategory: Ulduar
EndScriptData */

#include "precompiled.h"
#include "ulduar.h"
#include "TemporarySummon.h"
#include "escort_ai.h"

enum spells
{
    //Algalon's abillities
    SPELL_QUANTUMSTRIKE = 64395,
    SPELL_BIGBANG = 64443,
    SPELL_PHASE_PUNCH = 64412,
    SPELL_PHASE_SHIFT = 64417,
    SPELL_ALGALONS_BERSERK = 47008,
    SPELL_COSMIC_SMASH_MISSILE = 62304,
    SPELL_COSMIC_SMASH_VISUAL = 62300,
    SPELL_COSMIC_SMASH_DAMAGE = 62311,
    SPELL_DUAL_WIELD = 42459,
    SPELL_WIPE = 64487,

    //Black hole's abillities
    SPELL_BLACK_HOLE_EXPLOSION = 64122,
    SPELL_CONSTELLATION_PHASE_EFFECT = 65509,
    SPELL_SHADOW_FISURE         = 27810,
    SPELL_COSMIC_SMASH_DBM    = 64598,

    //living constellation abillities
    SPELL_ARCANE_BARRAGE = 64599,

    SPELL_AZEROTH_BEAM   = 64367,
};

enum NPCs
{
    NPC_COLLAPSING_STAR = 32955,
    NPC_BLACK_HOLE = 32953,
    NPC_VOID_ZONE = 34100,
    NPC_LIVING_CONSTELLATION = 33052,
    NPC_UNLEASHED_DARK_MATTER = 34097,
    NPC_MOB_ALGALON_STALKER_ASTEROID_TARGET_01 = 33104,
    NPC_MOB_ALGALON_STALKER_ASTEROID_TARGET_02 = 33105,
    NPC_BRANN_BRONZEBEARD = 34064,

    NPC_AZEROTH  = 34246,
};

enum GameObjects
{
    GO_SIGIL_DOOR_02 = 194911
};

enum Says
{
    SAY_INTRO_1                         = -1603106,
    SAY_INTRO_2                         = -1603107,
    SAY_INTRO_3                         = -1603108,

    SAY_ENGAGE                          = -1603109,
    SAY_AGGRO                           = -1603110,
    SAY_SLAY_1                          = -1603111,
    SAY_SLAY_2                          = -1603112,
    SAY_SUMMON_STAR                     = -1603113,
    SAY_BIG_BANG_1                      = -1603114,
    SAY_BIG_BANG_2                      = -1603115,
    SAY_PHASE_2                         = -1603116,
    SAY_BERSERK                         = -1603117,

    SAY_DESPAWN_1                       = -1603118,
    SAY_DESPAWN_2                       = -1603119,
    SAY_DESPAWN_3                       = -1603120,

    SAY_OUTRO_1                         = -1603121,
    SAY_OUTRO_2                         = -1603122,
    SAY_OUTRO_3                         = -1603123,
    SAY_OUTRO_4                         = -1603124,
    SAY_OUTRO_5                         = -1603125,
};

/************************************************ Algalon ***************************************************/

struct MANGOS_DLL_DECL boss_algalonAI : public ScriptedAI
{
    typedef std::list<uint64> GUIDList;
    ScriptedInstance* m_pInstance;

    //Creatures lists
    GUIDList m_lLivingConstelationsGUIDs;
    GUIDList m_lCollapsingStarsGUIDs;
    GUIDList m_lDarkMattersGUIDs;
    std::list<Creature*> lBlackHoles;
    std::list<Creature*> lVoidZones;

    //Algalon's abillities timers
    bool m_uiPhaseTwo;

    uint32 m_uiQuantumStrikeTimer;
    uint32 m_uiBigBangTimer;
    uint32 m_uiBigBangPhaseOutEffectTimer;
    uint32 m_uiPhasePunchTimer;
    uint32 m_uiAlgalonsBerserkTimer;
    uint32 m_uiCosmicSmashTimer;
    uint32 m_uiDualWieldTimer;

    uint32 m_uiSummonCollapsingStarTimer;
    uint32 m_uiSummonLivingConstellationTimer;

    //Black hole's abillities timers
    uint32 m_uiSummonUnleashedDarkMatterTimer;

    uint32 m_uiDespawnCheck;
    uint32 m_uiDespawnTime;
    uint32 m_uiEvadeTimer;
    bool despawned;

    uint32 m_uiIntroTimer;
    uint8 m_uiIntroStep;
    bool intro;

    uint32 m_uiPhaseDmgTimer;

    bool firstConst;

    void Reset()
    {
        m_uiPhaseTwo = false;

        m_uiQuantumStrikeTimer = urand(3000, 6000);
        m_uiBigBangTimer = 90000;
        m_uiBigBangPhaseOutEffectTimer = 99000;
        m_uiPhasePunchTimer = 15000;
        m_uiSummonCollapsingStarTimer = 15000;
        m_uiSummonLivingConstellationTimer = 70000;
        m_uiSummonUnleashedDarkMatterTimer = 5000;
        m_uiCosmicSmashTimer = 30000;
        m_uiAlgalonsBerserkTimer = 360000;
        m_uiDualWieldTimer = 900;
        m_uiDespawnCheck = 1000;
        m_uiDespawnTime = 0;
        despawned = false;
        m_uiIntroTimer = 10000;
        m_uiIntroStep = 0;
        firstConst = true;
        m_uiPhaseDmgTimer = 2000;
        intro = (m_pInstance->GetData(TYPE_ALGALON_EVENT) == 0);
        if(intro)
        {
            m_creature->SetVisibility(VISIBILITY_OFF);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }
        m_uiEvadeTimer = 0;
    }

    boss_algalonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    void DespawnAll()
    {
        for (GUIDList::const_iterator itr = m_lCollapsingStarsGUIDs.begin(); itr != m_lCollapsingStarsGUIDs.end(); itr++)
            if (Creature* temp_star = m_creature->GetMap()->GetCreature(*itr))
                ((TemporarySummon*)temp_star)->UnSummon();
        m_lCollapsingStarsGUIDs.clear();

        for (GUIDList::const_iterator itr = m_lLivingConstelationsGUIDs.begin(); itr != m_lLivingConstelationsGUIDs.end(); itr++)
            if (Creature* temp_constellation = m_creature->GetMap()->GetCreature(*itr))
                ((TemporarySummon*)temp_constellation)->UnSummon();
        m_lLivingConstelationsGUIDs.clear();

        for (GUIDList::const_iterator itr = m_lDarkMattersGUIDs.begin(); itr != m_lDarkMattersGUIDs.end(); itr++)
            if (Creature* temp_dark_matter = m_creature->GetMap()->GetCreature(*itr))
                ((TemporarySummon*)temp_dark_matter)->UnSummon();
        m_lDarkMattersGUIDs.clear();

        GetCreatureListWithEntryInGrid(lBlackHoles, m_creature, NPC_BLACK_HOLE, 100.0f);
        for (std::list<Creature*>::iterator itr = lBlackHoles.begin(); itr != lBlackHoles.end(); itr++)
            ((TemporarySummon*)(*itr))->UnSummon();
        lBlackHoles.clear();

        GetCreatureListWithEntryInGrid(lVoidZones, m_creature, NPC_VOID_ZONE, 100.0f);
        for (std::list<Creature*>::iterator itr = lVoidZones.begin(); itr != lVoidZones.end(); itr++)
            ((TemporarySummon*)(*itr))->UnSummon();
        lVoidZones.clear();
    }

    void PhaseOutAll()
    {
        Map::PlayerList const& lPlayers = m_pInstance->instance->GetPlayers();
        for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
            if (Player* pPlayer = itr->getSource())
                pPlayer->SetPhaseMask(1, true);
    }

    void JustDied(Unit* pKiller)
    {
        GameObject* pGo = GetClosestGameObjectWithEntry(m_creature, GO_AZEROTH_GLOBE, INTERACTION_DISTANCE);
        if (pGo)
            pGo->SetGoState(GO_STATE_READY);
        DespawnAll();
        PhaseOutAll();
        m_pInstance->SetData(TYPE_ALGALON, DONE);
    }

    void Aggro(Unit* pWho)
    {
    /*   pGo = GetClosestGameObjectWithEntry(m_creature, GO_UNIVERSE_FLOOR_ARCHIVUM, 200);
        if (pGo)
            pGo->SetGoState(GO_STATE_READY);

        pGo = GetClosestGameObjectWithEntry(m_creature, GO_UNIVERSE_FLOOR_CELESTIAL, 200);
        if (pGo)
            pGo->SetGoState(GO_STATE_ACTIVE);*/

        m_pInstance->SetData(TYPE_ALGALON, IN_PROGRESS);
        m_pInstance->SetData(TYPE_ALGALON_PULL_TIME, time(0));
        DoScriptText(SAY_AGGRO, m_creature);
        
    }

    void EnterEvadeMode()
    {
        DespawnAll();
        PhaseOutAll();
        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop(true);

        m_creature->InterruptNonMeleeSpells(false);
        DoCast(m_creature, SPELL_WIPE);
        m_uiAlgalonsBerserkTimer = 360000;
        m_uiBigBangTimer = 90000;
        m_uiEvadeTimer = 5000;
        //ScriptedAI::EnterEvadeMode();
    }

    void AttackStart(Unit* pWho)
    {
        if(despawned || intro)
            return;
        ScriptedAI::AttackStart(pWho);
    }

    void JustReachedHome()
    {
        GameObject* pGo = GetClosestGameObjectWithEntry(m_creature, GO_AZEROTH_GLOBE, INTERACTION_DISTANCE);
        if (pGo)
            pGo->SetGoState(GO_STATE_READY);
        DespawnAll();
        PhaseOutAll();
        m_pInstance->SetData(TYPE_ALGALON, FAIL);
        m_pInstance->SetData(TYPE_ALGALON_EVENT, 0);
        ((TemporarySummon*)m_creature)->UnSummon();
    }

    void KilledUnit()
    {
        switch (rand()%2)
        {
            case 0: DoScriptText(SAY_SLAY_1, m_creature); break;
            case 1: DoScriptText(SAY_SLAY_2, m_creature); break;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if(despawned)
            return;

        if(m_uiEvadeTimer)
        {
            if(m_uiEvadeTimer <= uiDiff)
            {
                ScriptedAI::EnterEvadeMode();
                m_uiEvadeTimer = 0;
            }else m_uiEvadeTimer -= uiDiff;
        }

        if(m_uiDespawnCheck <= uiDiff)
        {
            if(!m_uiDespawnTime)
                m_uiDespawnTime = m_pInstance->GetData(TYPE_ALGALON_PULL_TIME);
            if(m_uiDespawnTime && m_uiDespawnTime <= time(0))
            {
                EnterEvadeMode();
                m_creature->SetVisibility(VISIBILITY_OFF);
                despawned = true;
            }
            m_uiDespawnCheck = 1000;
        }else m_uiDespawnCheck -= uiDiff;

        if(intro)
        {
            if(m_uiIntroTimer <= uiDiff)
            {
                switch(m_uiIntroStep)
                {
                    case 0:
                        m_creature->SetVisibility(VISIBILITY_ON);
                        m_uiIntroTimer = 1000;
                        break;
                    case 1:
                    {
                        float x, y, z;
                        m_creature->GetPosition(x, y, z);
                        x += cos(m_creature->GetOrientation())*15;
                        y += sin(m_creature->GetOrientation())*15;
                        m_creature->SummonCreature(NPC_AZEROTH, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 20000);
                        m_uiIntroTimer = 4000;
                        break;
                    }
                    case 2:
                        m_creature->CastSpell(m_creature, SPELL_AZEROTH_BEAM, true);
                        m_uiIntroTimer = 14000;
                        break;
                    case 3:
                    {
                        GameObject* pGo = GetClosestGameObjectWithEntry(m_creature, GO_AZEROTH_GLOBE, 200);
                        if (pGo)
                            pGo->SetGoState(GO_STATE_ACTIVE);
                        pGo = GetClosestGameObjectWithEntry(m_creature, GO_UNIVERSE_FLOOR_CELESTIAL, 200);
                        if (pGo)
                            pGo->SetGoState(GO_STATE_ACTIVE);
                        m_uiIntroTimer = 18000;
                        break;
                    }
                    case 4:
                        m_creature->InterruptNonMeleeSpells(false);
                        m_creature->RemoveAllAuras();
                        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        intro = false;
                        break;
                }
                ++m_uiIntroStep;
            }else m_uiIntroTimer -= uiDiff;
            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Dual wield
        if (m_uiDualWieldTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_DUAL_WIELD);
            m_uiDualWieldTimer = 1800;
        }
        else
            m_uiDualWieldTimer -=uiDiff;

        //Phase 2
        if (!m_uiPhaseTwo && m_creature->GetHealthPercent() < 20.0f)
        {
            m_uiPhaseTwo = true;
            DoScriptText(SAY_PHASE_2, m_creature);
            DespawnAll();
            Creature* temp_black_hole = NULL;
            Creature* temp_void_zone = NULL;

            float angle = 0;
            for (uint8 i = 0; i < 4; ++i)
            {
                angle += M_PI_F/2;
                float x = 1632.25f + cos(angle)*15;
                float y = -307.548f + sin(angle)*15;
                m_creature->SummonCreature(NPC_BLACK_HOLE, x, y, 417.327f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
                m_creature->SummonCreature(NPC_VOID_ZONE, x, y, 417.327f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
            }
        }

        //Berserk
        if (m_uiAlgalonsBerserkTimer < uiDiff)
        {
            m_creature->InterruptNonMeleeSpells(false);
            DoScriptText(SAY_BERSERK, m_creature);
            DoCast(m_creature, SPELL_ALGALONS_BERSERK, true);
            DoCast(m_creature, SPELL_WIPE);
            m_uiAlgalonsBerserkTimer = 360000;
            m_uiBigBangTimer = 90000;
        }
        else
            m_uiAlgalonsBerserkTimer -= uiDiff;

        //Quantum strike
        if (m_uiQuantumStrikeTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_QUANTUMSTRIKE);
            m_uiQuantumStrikeTimer = urand(3000, 6000);
        }
        else
             m_uiQuantumStrikeTimer -= uiDiff;

        if(m_uiPhaseDmgTimer <= uiDiff)
        {
            Map::PlayerList const& lPlayers = m_pInstance->instance->GetPlayers();
            for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if(pPlayer->IsInWorld() && pPlayer->isAlive() && pPlayer->GetPhaseMask() == 16)
                        m_creature->DealDamage(pPlayer, 2000, NULL, SPELL_DIRECT_DAMAGE, SpellSchoolMask(64), NULL, true);
                }
            }
            m_uiPhaseDmgTimer = 2000;
        }else m_uiPhaseDmgTimer -= uiDiff;

        //Big Bang
        if (m_uiBigBangTimer < uiDiff)
        {
            switch (rand()%2)
            {
                case 0: DoScriptText(SAY_BIG_BANG_1, m_creature); break;
                case 1: DoScriptText(SAY_BIG_BANG_2, m_creature); break;
            }
            m_creature->InterruptNonMeleeSpells(false);
            DoCast(m_creature->getVictim(), SPELL_BIGBANG);
            m_uiBigBangTimer = 90000;
        }
        else
            m_uiBigBangTimer -= uiDiff;

        if (m_uiBigBangPhaseOutEffectTimer < uiDiff)
        {
            PhaseOutAll();
            m_uiBigBangPhaseOutEffectTimer = 90000;
        }
        else
            m_uiBigBangPhaseOutEffectTimer -= uiDiff;


        //Phase Punch
        if (m_uiPhasePunchTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_PHASE_PUNCH);
            m_uiPhasePunchTimer = 15000;
        }
        else
            m_uiPhasePunchTimer -= uiDiff;

        //Cosmic Smash
        if (m_uiCosmicSmashTimer < uiDiff)
        {
            Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM,0);
            m_creature->SummonCreature(NPC_MOB_ALGALON_STALKER_ASTEROID_TARGET_02,
                target->GetPositionX(), target->GetPositionY(), 417.327f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5500);
            m_creature->CastSpell(target, SPELL_SHADOW_FISURE, true);
            m_uiCosmicSmashTimer = 25000;
        }
        else
            m_uiCosmicSmashTimer -= uiDiff;

        //Summon Collapsing Stars
        if (!m_uiPhaseTwo)
        {
            if (m_uiSummonCollapsingStarTimer < uiDiff)
            {
                DoScriptText(SAY_SUMMON_STAR, m_creature);
                float angle = 0;
                for (uint32 i= 0; i<4; i++)
                {
                    angle += M_PI_F/2;
                    float x = 1632.25f + cos(angle) * urand(10, 38);
                    float y = -307.548f + sin(angle) * urand(10, 38);

                    Creature* temp_creature = m_creature->SummonCreature(NPC_COLLAPSING_STAR, x, y, 428.327f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
                    m_lCollapsingStarsGUIDs.push_back(temp_creature->GetGUID());
                }
                m_uiSummonCollapsingStarTimer=m_uiBigBangTimer + 15000;
            }
            else
                m_uiSummonCollapsingStarTimer -= uiDiff;
        }

        //Summon Living Constellation
        if (!m_uiPhaseTwo)
        {
            if (m_uiSummonLivingConstellationTimer < uiDiff)
            {
                float angle = 0;
                for (uint8 i = 0; i < (firstConst ? 3 : 2); ++i)
                {
                    angle += M_PI_F/2;
                    float x = 1632.25f + cos(angle)*38;
                    float y = -307.548f + sin(angle)*38;

                    Creature* temp_constellation = m_creature->SummonCreature(NPC_LIVING_CONSTELLATION, x, y, 428.327f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0);
                    temp_constellation->AI()->AttackStart(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM,0));
                    m_lLivingConstelationsGUIDs.push_back(temp_constellation->GetGUID());
                }
                m_uiSummonLivingConstellationTimer=50000;
                firstConst = false;
            }
            else
                m_uiSummonLivingConstellationTimer -= uiDiff;
        }


        //Summon Unleashed Dark Matter
        if (m_uiPhaseTwo)
        {
            if (m_uiSummonUnleashedDarkMatterTimer < uiDiff)
            {
                float angle = 0;
                for (uint8 i = 0; i < 4; ++i)
                {
                    angle += M_PI_F/2;
                    float x = 1632.25f + cos(angle)*15;
                    float y = -307.548f + sin(angle)*15;
                    Creature* temp_dark_matter = m_creature->SummonCreature(NPC_UNLEASHED_DARK_MATTER, x, y, 417.327f, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0);
                    temp_dark_matter->AI()->AttackStart(m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM,0));
                    m_lDarkMattersGUIDs.push_back(temp_dark_matter->GetGUID());
                }
                m_uiSummonUnleashedDarkMatterTimer = 30000;
            }
            else
                m_uiSummonUnleashedDarkMatterTimer -= uiDiff;
        }

        //Update threat list
       /* ThreatList const& tList = m_creature->getThreatManager().getThreatList();
        for (ThreatList::const_iterator itr = tList.begin();itr != tList.end(); ++itr)
        {
            Unit* pUnit = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid());

            if (pUnit && m_creature->getThreatManager().getThreat(pUnit))
                if (pUnit->GetPhaseMask() == 16)
                    m_creature->getThreatManager().modifyThreatPercent(pUnit, -100);
        }*/

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_algalon(Creature* pCreature) 
{
    return new boss_algalonAI(pCreature);
}

/****************************************** Collapsing Star *************************************************/

struct MANGOS_DLL_DECL mob_collapsing_starAI : public ScriptedAI
{
    uint32 m_uiSetHealthTimer;

    mob_collapsing_starAI(Creature* pCreature) : ScriptedAI(pCreature)  
    {
        Reset();
    }

    void Reset()
    {
        m_uiSetHealthTimer = 1000;
    }

    void AttackStart(Unit* pWho)
    {

    }

    void JustDied(Unit* pKiller)
    {
        m_creature->SummonCreature(NPC_BLACK_HOLE, m_creature->GetPositionX(), m_creature->GetPositionY(), 417.327f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
        m_creature->SummonCreature(NPC_VOID_ZONE, m_creature->GetPositionX(), m_creature->GetPositionY(), 417.327f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN, 0);
        ((TemporarySummon*)m_creature)->UnSummon();
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_uiSetHealthTimer < uiDiff)
        {
            m_creature->DealDamage(m_creature, 882, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            m_uiSetHealthTimer = 1000;
        }
        else
            m_uiSetHealthTimer -= uiDiff;
    }
};

CreatureAI* GetAI_mob_collapsing_star(Creature* pCreature)
{
    return new mob_collapsing_starAI(pCreature);
}

/****************************************** Living Constellation ********************************************/

struct MANGOS_DLL_DECL mob_living_constellationAI : public ScriptedAI
{
    uint32 m_uiArcaneBarrageTimer;

    mob_living_constellationAI (Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset()
    {
        m_uiArcaneBarrageTimer = urand(5000,10000);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_uiArcaneBarrageTimer < uiDiff)
        {
            Unit* target = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM,0);
            if (target)
                DoCastSpellIfCan(target, SPELL_ARCANE_BARRAGE);
            m_uiArcaneBarrageTimer = urand(5000,10000);
        }
        else
            m_uiArcaneBarrageTimer -= uiDiff;
    }
};

CreatureAI* GetAI_mob_living_constellation(Creature* pCreature)
{
    return new mob_living_constellationAI(pCreature);
}

/****************************************** Black Hole ********************************************************/

struct MANGOS_DLL_DECL mob_black_holeAI : public ScriptedAI
{
    uint32 m_uiConstellationPhaseEffectTimer;

    mob_black_holeAI (Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
        DoCastSpellIfCan(m_creature, SPELL_BLACK_HOLE_EXPLOSION);
    }

    void Reset()
    {
        m_uiConstellationPhaseEffectTimer = 1000;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_uiConstellationPhaseEffectTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_CONSTELLATION_PHASE_EFFECT);
            Unit* target = GetClosestCreatureWithEntry(m_creature, NPC_LIVING_CONSTELLATION, 5.0f);
            if (target)
            {
                Unit* tempVoidZone = GetClosestCreatureWithEntry(m_creature, NPC_VOID_ZONE, 5.0f);
                ((TemporarySummon*)target)->UnSummon();
                ((TemporarySummon*)tempVoidZone)->UnSummon();
                ((TemporarySummon*)m_creature)->UnSummon();
            }
            m_uiConstellationPhaseEffectTimer = 1000;
        }
        else
            m_uiConstellationPhaseEffectTimer -= uiDiff;
    }
};

CreatureAI* GetAI_mob_black_hole(Creature* pCreature)
{
    return new mob_black_holeAI(pCreature);
}

/****************************************** Meteor Stalker 02 *************************************************/

struct MANGOS_DLL_DECL mob_Algalon_Stalker_Asteroid_Target_02_AI : public ScriptedAI
{
    uint32 m_uiCosmicSmashDamageTimer;

    mob_Algalon_Stalker_Asteroid_Target_02_AI (Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    void Reset()
    {
        m_uiCosmicSmashDamageTimer = 4000;
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_uiCosmicSmashDamageTimer < uiDiff)
        {
            DoCast(m_creature, SPELL_COSMIC_SMASH_MISSILE, true);
            m_uiCosmicSmashDamageTimer = 4000;
        }
        else
            m_uiCosmicSmashDamageTimer -= uiDiff;

    }
};

CreatureAI* GetAI_mob_Algalon_Stalker_Asteroid_Target_02_AI(Creature* pCreature)
{
    return new mob_Algalon_Stalker_Asteroid_Target_02_AI(pCreature);
}

/****************************************** Doors ***********************************************************/

bool GOUse_go_planetarium_access(Player* pPlayer, GameObject* pGo)
{
    /*if(pGo->GetInstanceData()->GetData(TYPE_ALGALON_EVENT) == 0)
    {
        Creature *pAlgalon = pGo->SummonCreature(NPC_ALGALON, 1632.25f, -307.548f, 417.327f, 1.5f, TEMPSUMMON_MANUAL_DESPAWN, 0);
        pAlgalon->SetRespawnDelay(604800);

        GameObject* sigilDoor02 = GetClosestGameObjectWithEntry(pGo, GO_SIGIL_DOOR_02, 100.0f);
        sigilDoor02->Use(pPlayer);
        pGo->GetInstanceData()->SetData(TYPE_ALGALON_EVENT, 1);
    }*/
    return false;
}

/****************************************** Brann Bronzebeard ***********************************************/
/*
struct MANGOS_DLL_DECL mob_Brann_Bronzebeard_AI : public ScriptedAI
{
    mob_Brann_Bronzebeard_AI (Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
        m_creature->SetVisibility(VISIBILITY_OFF);
    }

    void Reset()
    {
    }

    void MovementInform(uint32 uiType, uint32 uiPointId)
    {
        switch (uiPointId)
        {
            case 6:
                m_creature->SummonCreature(NPC_ALGALON, 1632.25f, -307.548f, 417.327f, 1.5f, TEMPSUMMON_MANUAL_DESPAWN, 0);
                break;

            case 11:
                m_creature->GetMotionMaster()->MoveIdle();
                m_creature->SetVisibility(VISIBILITY_OFF);
                break;
        }
    }
};

CreatureAI* GetAI_mob_Brann_Bronzebeard_AI(Creature* pCreature)
{
    return new mob_Brann_Bronzebeard_AI(pCreature);
}*/

bool GossipHello_mob_brann_open(Player* pPlayer, Creature* pCreature)
{
    bool m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
    if(pCreature->GetInstanceData()->GetData(TYPE_ALGALON_EVENT) != 0 && pCreature->GetInstanceData()->GetData(TYPE_ALGALON) != DONE)
        return false;

    bool hasItem = false;
    if (m_bIsRegularMode)
    {
        if(pPlayer->HasItemCount(45796, 1) || pPlayer->HasItemCount(45798, 1))
            hasItem = true;
    }else{
        if(pPlayer->HasItemCount(45798, 1))
            hasItem = true;
    }
    if(!hasItem)
        return false;

    
    pPlayer->ADD_GOSSIP_ITEM(0, "We are ready", GOSSIP_SENDER_MAIN, 1);
    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
    return true;
}

bool GossipSelect_mob_brann_open(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    pPlayer->CLOSE_GOSSIP_MENU();
    if(pCreature->GetInstanceData()->GetData(TYPE_ALGALON_EVENT) != 0)
        return false;

    bool m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();

    bool hasItem = false;
    if (m_bIsRegularMode)
    {
        if(pPlayer->HasItemCount(45796, 1) || pPlayer->HasItemCount(45798, 1))
            hasItem = true;
    }else{
        if(pPlayer->HasItemCount(45798, 1))
            hasItem = true;
    }
    if(!hasItem)
        return false;

    if (uiAction == 1)
    {
        Creature *pAlgalon = pCreature->SummonCreature(NPC_ALGALON, 1632.25f, -307.548f, 417.327f, 1.5f, TEMPSUMMON_MANUAL_DESPAWN, 0);
        pAlgalon->SetRespawnDelay(604800);

        GameObject* sigilDoor02 = GetClosestGameObjectWithEntry(pCreature, GO_CELESTIAL_DOOR, 100.0f);
        sigilDoor02->Use(pPlayer);
        pCreature->GetInstanceData()->SetData(TYPE_ALGALON_EVENT, 1);
    }

    return true;
}

/****************************************** Add scripts *****************************************************/

void AddSC_boss_algalon()
{
    Script* NewScript;

    NewScript = new Script;
    NewScript->Name = "boss_algalon";
    NewScript->GetAI = &GetAI_boss_algalon;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "mob_collapsing_star";
    NewScript->GetAI = &GetAI_mob_collapsing_star;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "mob_living_constellation";
    NewScript->GetAI = &GetAI_mob_living_constellation;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "mob_black_hole";
    NewScript->GetAI = &GetAI_mob_black_hole;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "mob_Algalon_Stalker_Asteroid_Target_02";
    NewScript->GetAI = &GetAI_mob_Algalon_Stalker_Asteroid_Target_02_AI;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "go_planetarium_access";
    NewScript->pGOHello = &GOUse_go_planetarium_access;
    NewScript->RegisterSelf();

    NewScript = new Script;
    NewScript->Name = "mob_Brann_Bronzebeard";
    //NewScript->GetAI = &GetAI_mob_Brann_Bronzebeard_AI;
    NewScript->pGossipHello = &GossipHello_mob_brann_open;
    NewScript->pGossipSelect = &GossipSelect_mob_brann_open;
    NewScript->RegisterSelf();
}
