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
SDName: boss_devourer_of_souls
SD%Complete: 0%
SDComment:
SDCategory: The Forge of Souls
EndScriptData */

#include "precompiled.h"
#include "forge_of_souls.h"

enum
{
    SAY_AGGRO               = -1632006,
    SAY_WAILING_SOULS       = -1632007,
    SAY_UNLEASH_SOULS       = -1632008,
    SAY_KILL1               = -1632009,
    SAY_KILL2               = -1632010,
    SAY_DEATH               = -1632011,

    EMOTE_MIRRORED_SOULS    = -1632012,
    EMOTE_UNLEASH_SOULS     = -1632013,

    SPELL_WELL_OF_SOULS     = 68854,
    SPELL_PHANTOM_BLAST     = 68982,
    SPELL_PHANTOM_BLAST_H   = 70322,
    SPELL_UNLEASHED_SOULS   = 68939,

    SPELL_MIRRORED_SOULS    = 69051,

    NPC_WELL_OF_SOULS       = 36536,
};

/*######
## boss_bronjahm
######*/

static const float wps[][4] =
{
    {5620.47, 2454.46, 705.9, 0.89},
    {5632.42, 2471.18, 708.7, 0},
};

struct MANGOS_DLL_DECL boss_devourer_of_soulsAI : public ScriptedAI
{
    boss_devourer_of_soulsAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;

    uint32 m_uiPhantomBlastTimer;
    uint32 m_uiMirroredSoulsTimer;
    uint32 m_uiUnleashedSoulsTimer;
    uint32 m_uiWellOfSoulsTimer;

    Player *mirrorPlr;
    uint8 mirrorStage;

    void Reset()
    {
        m_uiPhantomBlastTimer = 3000;
        m_uiMirroredSoulsTimer = 10000;
        m_uiUnleashedSoulsTimer = 15000;
        m_uiWellOfSoulsTimer = 6000;
        mirrorPlr = NULL;
        mirrorStage = 0;
    }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_AGGRO, m_creature);
    }

    void JustDied(Unit* pKiller)
    {
        DoScriptText(SAY_DEATH, m_creature);
        Creature *pGuide = m_creature->SummonCreature((m_pInstance->GetData(TYPE_FACTION) ? NPC_SYLVANAS : NPC_JAINA), wps[0][0], wps[0][1], wps[0][2],
                                                      wps[0][3], TEMPSUMMON_MANUAL_DESPAWN, 0);
        pGuide->GetMotionMaster()->MovePoint(1, wps[1][0], wps[1][1], wps[1][2]);
    }

    void KilledUnit(Unit* pVictim)
    {
        DoScriptText(urand(0,1) ? SAY_KILL1 : SAY_KILL2, m_creature);
    }

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
    {
        if (mirrorStage != 2 || !mirrorPlr || !mirrorPlr->IsInWorld() || !mirrorPlr->isAlive())
            return;
        uint32 dmg = m_bIsRegularMode ? uiDamage*0.3f : uiDamage*0.45f;
        pDoneBy->DealDamage(mirrorPlr, dmg, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if(pSummoned->GetEntry() == NPC_JAINA || pSummoned->GetEntry() == NPC_SYLVANAS)
            return;
        pSummoned->setFaction(m_creature->getFaction());
        pSummoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        pSummoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        pSummoned->ForcedDespawn(10000);

        Player* plr = m_creature->SelectAttackingPlayer(ATTACKING_TARGET_RANDOM, 0);
        if (plr)
        {
            pSummoned->AI()->AttackStart(plr);
            pSummoned->AddThreat(plr, 1000.0f);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiPhantomBlastTimer <= uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), m_bIsRegularMode ? SPELL_PHANTOM_BLAST : SPELL_PHANTOM_BLAST_H);
            m_uiPhantomBlastTimer = 3000;
        }else m_uiPhantomBlastTimer -= uiDiff;

        if (m_uiMirroredSoulsTimer <= uiDiff)
        {
            switch(mirrorStage)
            {
                case 0:
                {
                    Player* plr = m_creature->SelectAttackingPlayer(ATTACKING_TARGET_RANDOM, 0);
                    if (plr)
                    {
                        DoScriptText(EMOTE_MIRRORED_SOULS, m_creature);
                        DoCast(plr, SPELL_MIRRORED_SOULS);
                        mirrorPlr = plr;
                    }
                    m_uiMirroredSoulsTimer = 2000;
                    m_uiWellOfSoulsTimer = 6000;
                    ++mirrorStage;
                    break;
                }
                case 1:
                {
                    ++mirrorStage;
                    m_uiMirroredSoulsTimer = 8000;
                    break;
                }
                case 2:
                {
                    mirrorStage = 0;
                    m_uiMirroredSoulsTimer = 10000;
                }
            }
        }else m_uiMirroredSoulsTimer-= uiDiff;

        if (m_uiUnleashedSoulsTimer <= uiDiff)
        {
            DoCast(m_creature, SPELL_UNLEASHED_SOULS);
            DoScriptText(EMOTE_UNLEASH_SOULS, m_creature);
            DoScriptText(SAY_UNLEASH_SOULS, m_creature);
            m_uiUnleashedSoulsTimer = 17000;
            m_uiWellOfSoulsTimer = 6000;
        }else m_uiUnleashedSoulsTimer -= uiDiff;

        if (m_uiWellOfSoulsTimer <= uiDiff)
        {
            Player* plr = m_creature->SelectAttackingPlayer(ATTACKING_TARGET_RANDOM, 0);
            if (plr)
            {
                float x, y, z;
                plr->GetPosition(x, y, z);

                Creature *well = m_creature->SummonCreature(NPC_WELL_OF_SOULS, x, y, z, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000);
                if (well)
                    well->CastSpell(m_creature, SPELL_WELL_OF_SOULS, true);
                
                plr->GetNearPoint(m_creature, x, y, z, m_creature->GetObjectBoundingRadius(), 1.0f, M_PI_F);
                m_creature->NearTeleportTo(x, y, z, 0);
            }
            m_uiWellOfSoulsTimer = 6000;
        }else m_uiWellOfSoulsTimer -= uiDiff;
        

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_devourer_of_souls(Creature* pCreature)
{
    return new boss_devourer_of_soulsAI(pCreature);
}

void AddSC_boss_devourer_of_souls()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "boss_devourer_of_souls";
    newscript->GetAI = &GetAI_boss_devourer_of_souls;
    newscript->RegisterSelf();
}

