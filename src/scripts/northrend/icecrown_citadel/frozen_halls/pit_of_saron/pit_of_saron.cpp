
#include "precompiled.h"
#include "pit_of_saron.h"
#include "../../../../../../dep/recastnavigation/RecastDemo/Contrib/stb_image.h"

const float mobPosLeft[][3] =
{
   {450.228, 229.039, 528.709}, // lavo 37496 (37584)
   {451.704, 225.592, 528.709}, // lavo 37496 (37584)
   {448.348, 222.751, 528.708}, // lavo 37496 (37584)
   {446.051, 224.950, 528.708}, // lavo 37497 (37587)
   {443.588, 220.182, 528.708}, // lavo 37497 (37587)
};

const float mobPosRight[][3] =
{
   {448.292, 204.437, 528.706}, // pravo 37496 (37584)
   {451.279, 201.190, 528.706}, // pravo 37496 (37584)
   {450.860, 197.152, 528.707}, // pravo 37496 (37584)
   {448.359, 193.731, 528.708}, // pravo 37496 (37584)
   {446.766, 187.732, 528.707}, // pravo 37496 (37584)
   {449.695, 183.464, 528.706}, // pravo 37496 (37584)
   {445.419, 179.930, 528.707}, // pravo 37496 (37584)
   {445.702, 206.950, 528.707}, // pravo 37497 (37587)
   {445.088, 202.032, 528.706}, // pravo 37497 (37587)
   {445.348, 197.622, 528.706}, // pravo 37497 (37587)
   {443.485, 191.246, 528.754}, // pravo 37497 (37587)
};

const uint32 heroIds [][2]
{
    {37498, 37587},
    {37496, 37584},
    {37497, 37588},
};

const uint32 necrolytePos[2][3]
{
    {496.83, 198.89, 528.794}, // right
    {497.31, 247.85, 528.791}, // left
};


#define INTRO_MOB_COUNT_LEFT 5
#define INTRO_MOB_COUNT_RIGHT 11

INSERT INTO `scriptdev2`.`script_texts` (`entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES
('-1658901', 'Chm! Farther, not event fit to laber in the quarry. Relish these final moments, for soon you will be nothing more the mindless undead.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16748', '1', '0', '0', ''),
('-1658900', 'Intruders have entered the Master''s domain. Signal the alarms!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', 'Tyrranus opening'),
('-1658902', 'Heroes of the Alliance, attack!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16626', '1', '0', '0', ''),
('-1658903', 'Soldiers of the Horde, attack!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '17045', '1', '0', '0', ''),
('-1658904', 'Your last waking memory will be of agonazing pain!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16749', '1', '0', '0', ''),
('-1658905', 'Minions! Destroy these interlopers!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16751', '1', '0', '0', ''),
('-1658906', 'No, you monster!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16627', '1', '0', '0', ''),
('-1658907', 'Pathetic weaklings...', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '17046', '1', '0', '0', ''),
('-1658908', 'You will have to make your way across this quarry on your own.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16628', '1', '0', '0', ''),
('-1658909', 'You will have to battle your way threw this pit on your own.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '17047', '1', '0', '0', ''),
('-1658910', 'Free any Alliance slaves that you come across. We will most certainly need their assistence in battling Tyrannus. I will gather reinforcemens and join you on the other side of the quarry.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16629', '1', '0', '0', ''),
('-1658911', 'Free any Horde slaves that you come across. We will most certainly need their assistence in battling Tyrannus. I will gather reinforcemens and join you on the other side of the quarry.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '17048', '1', '0', '0', ''),
('-1658900', '', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', ''),
('-1658900', '', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', ''),
('-1658900', '', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', ''),
('-1658900', '', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', ''),
('-1658900', '', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, '16747', '1', '0', '0', ''),


enum
{
    SAY_TYRANNUS_INTRO1     = -1658900,
    SAY_TYRANNUS_INTRO2     = -1658902,
    SAY_TYRANNUS_INTRO3     = -1658904,
    SAY_TYRANNUS_INTRO3     = -1658905,

    SOUND_TYRRANNUS_TRANSFORM = 16750,

    SPELL_STRANGULATE       = 69413,
    SPELL_TURN_TO_UNDEAD    = 69350,
    SPELL_SYLVANAS_PWNT     = 59514,
    SPELL_JAINA_PWNT        = 70464,

    NPC_SKELETAL_SLAVE      = 36881,
};

const int32 say_guide[][2]
{
    {-1658902, -1658903},
    {-1658906, -1658907},
    {-1658908, -1658909},
    {-1658910, -1658911},
};

struct MANGOS_DLL_DECL mob_pos_guide_startAI : public ScriptedAI
{
    mob_pos_guide_startAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;

    uint32 m_uiExplodeTimer;
    uint32 m_uiEventTimer;
    uint8 faction;
    uint8 eventState;
    uint8 eventStateInternal;
    bool stopped;

    std::vector<Creature*> leftHerosList;
    std::vector<Creature*> rightHerosList;

    Creature *pTyrannus;

    void Reset()
    {
        if(m_pInstance)
            m_creature->ForcedDespawn();
        faction = m_pInstance->GetData(TYPE_FACTION);
        eventState = m_pInstance->GetData(TYPE_EVENT_STATE);
        stopped = false;
        switch(eventState)
        {
            case 0:
                SpawnIntro();
                m_uiEventTimer = 5000;
                eventStateInternal = 0;
                pTyrannus = m_creature->GetMap()->GetCreature(m_pInstance->GetData64(NPC_TYRANNUS_INTRO));
                if(!pTyrannus)
                    m_creature->ForcedDespawn();
                break;
            default:
                stopped = true;
                break;
        }
    }

    void SetEventState(uint32 state)
    {
         eventState = state;
         m_pInstance->SetData(TYPE_EVENT_STATE, state);
    }

    void SpawnIntro()
    {
        Creature *tmp;
        for(uint8 i = 0; i < INTRO_MOB_COUNT_LEFT; ++i)
        {
            tmp = m_creature->SummonCreature(heroIds[urand(0, 2)][faction],
                                       mobPosLeft[i][0], mobPosLeft[i][1], mobPosLeft[i][2],
                                       0.104f, TEMPSUMMON_DEAD_DESPAWN, 0);
            leftHerosList.push_back(tmp);
        }
        for(uint8 i = 0; i < INTRO_MOB_COUNT_RIGHT; ++i)
        {
            tmp = m_creature->SummonCreature(heroIds[urand(0, 2)][faction],
                                       mobPosRight[i][0], mobPosRight[i][1], mobPosRight[i][2],
                                       0.104f, TEMPSUMMON_DEAD_DESPAWN, 0);
            rightHerosList.push_back(tmp);
        }
        tmp = m_creature->SummonCreature(NPC_NECROLYTE, necrolytePos[0][0], necrolytePos[0][1], necrolytePos[0][2],
                                         0.104f, TEMPSUMMON_DEAD_DESPAWN, 0);
        rightHerosList.push_back(tmp);
        tmp = m_creature->SummonCreature(NPC_NECROLYTE, necrolytePos[1][0], necrolytePos[1][1], necrolytePos[1][2],
                                         0.104f, TEMPSUMMON_DEAD_DESPAWN, 0);
        leftHerosList.push_back(tmp);
    }

    void AttackStart(Unit* pWho)
    {
        return;
    }

    void DoAttack()
    {
        Creature *acolyte = rightHerosList[rightHerosList.size()-1];
        rightHerosList.pop_back();
        for(std::vector<Creture*>::iterator itr = rightHerosList.begin(); itr != rightHerosList.end(); ++itr)
            (*itr)->AI()->AttackStart(acolyte);

        acolyte = leftHerosList[leftHerosList.size()-1];
        leftHerosList.pop_back();
        for(std::vector<Creture*>::iterator itr = leftHerosList.begin(); itr != leftHerosList.end(); ++itr)
        {
            (*itr)->AI()->AttackStart(acolyte);
            // put everything into right
            rightHerosList.push_back(*itr);
        }
    }

    void DoStrangulate(bool up, uint32 time)
    {
        float x, y, z;
        for(std::vector<Creture*>::iterator itr = rightHerosList.begin(); itr != rightHerosList.end(); ++itr)
        {
            (*itr)->GetPosition(x, y, z);
            if(up)
            {
                (*itr)->CastSpell(*itr, SPELL_STRANGULATE, true);
                (*itr)->CombatStop();
                z += 10.0f;
            }
            else
            {
                (*itr)->RemoveAurasDueToSpell(SPELL_STRANGULATE);
                z -= 9.5f;
            }
            (*itr)->SendMonsterMove(x, y, z, SPLINETYPE_NORMAL, SPLINEFLAG_NONE, time);
            (*itr)->GetMap()->CreatureRelocation(x, y, z, 0.104f);
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if(stopped)
            return;
        
        if(m_uiEventTimer <= uiDiff)
        {
            switch(eventStateInternal)
            {
                case 0:
                   pTyrannus->AI()->DoAction(SAY_TYRANNUS_INTRO1);
                   m_uiEventTimer = 4000;
                   break;
               case 1:
                   pTyrannus->AI()->DoAction(SAY_TYRANNUS_INTRO2);
                   m_uiEventTimer = 12500;
                   break;
               case 2:
                   DoScriptText(say_guide[0][faction], m_creature);
                   m_uiEventTimer = 1500;
                   break;
               case 3:
                   DoAttack();
                   m_uiEventTimer = 4000;
                   break;
               case 4:
                   DoStrangulate(true, 4000);
                   m_uiEventTimer = 4000;
                   break;
               case 5:
                   pTyrannus->AI()->DoAction(SOUND_TYRRANNUS_TRANSFORM);
                   m_uiEventTimer = 7000;
                   break;
               case 6:
                   for(std::vector<Creture*>::iterator itr = rightHerosList.begin(); itr != rightHerosList.end(); ++itr)
                   {
                       pTyrannus->CastSpell(*itr, SPELL_TURN_TO_UNDEAD, false);
                       (*itr)->UpdateEntry(NPC_SKELETAL_SLAVE);
                   }
                   m_uiEventTimer = 1000;
                   break;
               case 7:
                   DoScriptText(say_guide[1][faction], m_creature);
                   DoStrangulate(false, 1000);
                   m_uiEventTimer = 2000;
                   break;
               case 8:
                   for(std::vector<Creture*>::iterator itr = rightHerosList.begin(); itr != rightHerosList.end(); ++itr)
                       (*itr)->AI()->AttackStart(m_creature);
                   m_uiEventTimer = 2000;
                   break;
               case 9:
                   if(faction)
                       DoCast(m_creature, SPELL_SYLVANAS_PWNT);
                   else
                   {
                       for(std::vector<Creture*>::iterator itr = rightHerosList.begin(); itr != rightHerosList.end(); ++itr)
                           m_creature->CastSpell(*itr, SPELL_JAINA_PWNT, true);
                   }
                   m_uiEventTimer = 2000;
                   break;
               case 10:
                   DoScriptText(say_guide[2][faction], m_creature);
                   m_uiEventTimer = 4000;
                   break;
               case 11:
                   DoScriptText(say_guide[3][faction], m_creature);
                   m_uiEventTimer = 10000;
                   break;
               case 12:
                   SetEventState(1);
                   stopped = true;
                   break;
            }
            ++eventStateInternal;
        }else m_uiEventTimer -= uiDiff;
    }
};

struct MANGOS_DLL_DECL mob_tyrannus_introAI : public ScriptedAI
{
    mob_tyrannus_introAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    void Reset()
    {
    }

    void AttackStart(Unit* pWho)
    {
        return;
    }

    void DoAction(uint32 action)
    {
        switch(action)
        {
            case SAY_TYRANNUS_INTRO1:
            case SAY_TYRANNUS_INTRO2:
            case SAY_TYRANNUS_INTRO3
                DoScriptText(action, m_creature);
                return;
            case SOUND_TYRRANNUS_TRANSFORM:
                DoPlaySoundToSet(this, action);
                return;
        }
    }
    
    void UpdateAI(const uint32 uiDiff)
    {
       
    }
};

CreatureAI* GetAI_mob_pos_guide_start(Creature* pCreature)
{
    return new mob_pos_guide_startAI(pCreature);
}

CreatureAI* GetAI_mob_tyrannus_intro(Creature* pCreature)
{
    return new mob_tyrannus_introAI(pCreature);
}

void AddSC_pit_of_saron()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "mob_pos_guide_start";
    newscript->GetAI = &GetAI_mob_pos_guide_start;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_tyrannus_intro";
    newscript->GetAI = &GetAI_mob_tyrannus_intro;
    newscript->RegisterSelf();
}