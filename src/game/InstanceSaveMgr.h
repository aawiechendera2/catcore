/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
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

#ifndef __InstanceSaveMgr_H
#define __InstanceSaveMgr_H

#include "Platform/Define.h"
#include "Policies/Singleton.h"
#include "ace/Thread_Mutex.h"
#include <list>
#include <map>
#include "Utilities/UnorderedMap.h"
#include "Database/DatabaseEnv.h"
#include "DBCEnums.h"
#include "ObjectGuid.h"

struct InstanceTemplate;
struct MapEntry;
class Player;
class Group;

class InstanceSave
{
    public:
        typedef std::set<uint64> PlrListSaves;
        InstanceSave(uint32 MapId, uint32 InstanceId, Difficulty difficulty, bool perm, uint32 encountersMask = 0);
        ~InstanceSave();

        uint32 GetGUID() const { return m_instanceGuid.GetCounter(); }
        ObjectGuid const& GetObjectGuid() const { return m_instanceGuid; }
        uint32 GetMapId() const { return m_mapId;}
        uint32 GetResetTime() const { return resetTime; }
        Difficulty GetDifficulty() const { return m_diff; }
        void SetPermanent(bool yes);
        void ExtendFor(uint64 guid);
        void RemoveExtended(uint64 guid);
        void SetResetTime(uint32 time) { resetTime = time; }
        bool IsExtended(uint64 guid) const { return (m_extended.find(guid) != m_extended.end()); }
        bool IsPermanent() const { return m_perm; }
        bool HasPlayers() const { return (m_players.size() > 0); }
        bool RemoveOrExtendPlayers(); //caled at reset
        uint32 GetEncounterMask() const { return m_encountersMask; }
        void AddEncounter(uint32 mask);
        void SetUsedByMapState(bool used) { m_usedByMap = used; }
        bool IsUsedByMap() const { return m_usedByMap; }

        bool LoadPlayers();
        void SaveToDb(bool players = false, bool data = false);
        void DeleteFromDb();
        void RemoveAndDelete();
        void AddPlayer(uint64 guid);
        void RemovePlayer(uint64 guid);
        void UpdateId(uint32 id);

    private:
        uint32 m_mapId;
        MapEntry const *mapEntry;
        ObjectGuid m_instanceGuid;
        Difficulty m_diff;
        bool m_perm;
        PlrListSaves m_players;
        PlrListSaves m_extended;
        uint32 resetTime; //timestamp
        uint32 m_encountersMask;
        bool m_usedByMap;
};

class MANGOS_DLL_DECL InstanceSaveManager : public MaNGOS::Singleton<InstanceSaveManager, MaNGOS::ClassLevelLockable<InstanceSaveManager, ACE_Thread_Mutex> >
{
    public:
        typedef std::map<uint32, InstanceSave*> InstanceSaveMap;
        typedef std::map<uint32, const char*> InstanceEncounterMap;
        InstanceSaveManager();
        ~InstanceSaveManager();

        void CheckResetTime();
        void LoadSavesFromDb();
        void LoadInstanceEncounters();
        void PackInstances();

        const char *GetEncounterName(Creature *creature);

        InstanceSave* CreateInstanceSave(uint16 mapId, uint32 id, Difficulty difficulty, bool perm);
        void DeleteSave(uint32 id)
        {
            InstanceSaveMap::iterator itr = m_saves.find(id);
            if(itr == m_saves.end())
                return;
            itr->second->RemoveAndDelete();
    //        delete itr->second;
            m_saves.erase(itr);
        }
        InstanceSave* GetInstanceSave(uint32 id)
        {
            InstanceSaveMap::iterator itr = m_saves.find(id);
            if(itr == m_saves.end())
                return NULL;
            return itr->second;
        }
        //Called every hour or at startup
        void CheckResetTimes();

    private:
        InstanceSaveMap m_saves;
        InstanceEncounterMap m_encounters;
};

#define sInstanceSaveMgr MaNGOS::Singleton<InstanceSaveManager>::Instance()
#endif
