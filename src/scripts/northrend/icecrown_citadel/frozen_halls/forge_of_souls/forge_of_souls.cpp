#include "precompiled.h"
#include "forge_of_souls.h"

static const int32 textsStart[][2] =
{
    {-1632501, 0},  // 0
    {-1632502, -1632510}, //1
    {-1632503, 0}, //2
    {-1632504, -1632511}, //3
    {-1632505, -1632512}, //4
    {-1632506, -1632513}, //5
    {-1632507, -1632514}, //6
    {-1632508, -1632515}, //7
    {0, 0}, //8
    {0, 0}, //9
    {-1632509, -1632516}, //10
};

static const float wps[][3] =
{
    {5632.42, 2471.18, 708.7},
    {5664.66, 2557.83, 714.7},
    {5681.49, 2532.47, 714.7},
    {5683.90, 2534.93, 715.7},
};

const float mobPos[][3] =
{
   {5618.667, 2467.045, 706.439}, // left
   {5620.421, 2465.423, 706.481}, // left
   {5622.290, 2467.568, 707.298}, // left
   {5620.356, 2469.232, 707.219}, // left
   {5624.031, 2469.501, 708.034}, // left
   {5622.113, 2471.150, 707.956}, // left
   {5625.774, 2471.318, 708.696}, // left
   {5624.202, 2473.573, 708.696}, // left
   {5630.368, 2456.928, 706.335}, // right
   {5632.578, 2455.501, 706.294}, // right
   {5631.958, 2459.274, 707.150}, // right
   {5634.169, 2457.740, 707.077}, // right
   {5633.394, 2461.349, 707.866}, // right
   {5635.500, 2460.072, 707.840}, // right
   {5634.972, 2463.736, 708.677}, // right
   {5637.096, 2462.733, 708.696}, // right
};

const uint32 heroIds [][2] =
{
    {37498, 37587},
    {37496, 37584},
    {37497, 37588},
};

#define MOB_COUNT 16

struct MANGOS_DLL_DECL mob_fos_guideAI : public ScriptedAI
{
    mob_fos_guideAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsRegularMode;

    uint32 m_uiEventTimer;
    uint8 faction;
    uint8 eventState;
    uint8 eventStateInternal;
    bool stopped;

    std::vector<Creature*> mobList;
    void Reset()
    {
        if(!m_pInstance)
        {
            m_creature->ForcedDespawn();
            return;
        }
        faction = m_pInstance->GetData(TYPE_FACTION);
        eventState = m_pInstance->GetData(TYPE_EVENT_STATE);
        switch(eventState)
        {
            case 0:
                eventStateInternal = 0;
                m_uiEventTimer = 5000;
                break;
            case 1:
                m_creature->RemoveSplineFlag(SPLINEFLAG_WALKMODE);
                eventStateInternal = 10;
                m_uiEventTimer = 5000;
                Spawn();
                break;
        }
        
        stopped = false;
    }

    void SetEventState(uint32 state)
    {
         eventState = state;
         m_pInstance->SetData(TYPE_EVENT_STATE, state);
    }

    void AttackStart(Unit* pWho)
    {
        return;
    }

    void Spawn()
    {
        Creature *tmp;
        for(uint8 i = 0; i < MOB_COUNT; ++i)
        {
            tmp = m_creature->SummonCreature(heroIds[urand(0, 2)][faction], mobPos[i][0], mobPos[i][1], mobPos[i][2],
                                       0.104f, TEMPSUMMON_DEAD_DESPAWN, 0);
            mobList.push_back(tmp);
            tmp->RemoveSplineFlag(SPLINEFLAG_WALKMODE);
        }
    }

    void MovementInform(uint32 uiMoveType, uint32 uiPointId)
    {
        if(uiMoveType != POINT_MOTION_TYPE)
            return;
        m_creature->GetMotionMaster()->Clear(false, true);
        switch(uiPointId)
        {
            case 3:
                m_creature->GetMotionMaster()->MovePoint(4, wps[2][0], wps[2][1], wps[2][2]);
                return;
            case 4:
                m_creature->GetMotionMaster()->MovePoint(5, wps[3][0], wps[3][1], wps[3][2]);
                return;
            case 5:
                m_creature->SetVisibility(VISIBILITY_OFF);
                return;
        }
        m_creature->GetMotionMaster()->MoveIdle();
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
                    SetEventState(1);
                    m_uiEventTimer = 0;
                    if(faction == 0)
                        m_uiEventTimer = 7000;
                    else
                    {
                        ++eventStateInternal;
                        return;
                    }
                     break;
                case 1: m_uiEventTimer = faction ? 12000 :  9000; break;
                case 2:
                    m_uiEventTimer = 0;
                    if(faction == 0)
                        m_uiEventTimer = 8000;
                    else
                    {
                        ++eventStateInternal;
                        return;
                    }
                    break;
                case 3: m_uiEventTimer = faction ? 11000 : 10000; break;
                case 4: m_uiEventTimer = faction ? 11000 :  9000; break;
                case 5: m_uiEventTimer = faction ? 12000 :  12000; break;
                
                case 6: m_uiEventTimer = faction ? 8000  :  8000; break;
                case 7: m_uiEventTimer = faction ? 3000  :  5000; break;
                case 8: stopped = true; return;

                case 10:
                    m_uiEventTimer = 13000;
                    break;
                case 11:
                    m_creature->GetMotionMaster()->Clear(false, true);
                    m_creature->GetMotionMaster()->MovePoint(3, wps[1][0], wps[1][1], wps[1][2]);
                    for(std::vector<Creature*>::iterator itr = mobList.begin(); itr != mobList.end(); ++itr)
                        (*itr)->GetMotionMaster()->MovePoint(3, wps[3][0], wps[3][1], wps[3][2]);
                    stopped = true;
                    return;
            }
            DoScriptText(textsStart[eventStateInternal][faction], m_creature);
            ++eventStateInternal;
        }else m_uiEventTimer -= uiDiff;
    }
};

CreatureAI* GetAI_mob_fos_guide(Creature* pCreature)
{
    return new mob_fos_guideAI(pCreature);
}

void AddSC_forge_of_souls()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "mob_fos_guide";
    newscript->GetAI = &GetAI_mob_fos_guide;
    newscript->RegisterSelf();
}