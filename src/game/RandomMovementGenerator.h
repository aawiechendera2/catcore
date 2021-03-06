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

#ifndef MANGOS_RANDOMMOTIONGENERATOR_H
#define MANGOS_RANDOMMOTIONGENERATOR_H

#include "MovementGenerator.h"
#include "DestinationHolder.h"
#include "Traveller.h"
#include "Unit.h"

template<class T>
class MANGOS_DLL_SPEC RandomMovementGenerator
: public MovementGeneratorMedium< T, RandomMovementGenerator<T> >
{
    public:
        explicit RandomMovementGenerator(const Unit &) : i_nextMoveTime(0) {}

        void _setRandomLocation(T &);
        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);
        void UpdateMapPosition(uint32 mapid, float &x ,float &y, float &z)
        {
            i_destinationHolder.GetLocationNow(mapid, x,y,z);
        }
        MovementGeneratorType GetMovementGeneratorType() const { return RANDOM_MOTION_TYPE; }

        bool GetResetPosition(T&, float& x, float& y, float& z);
        bool GetDestination(float& x, float& y, float& z) const
        {
            if (!i_destinationHolder.HasDestination()) return false;
            i_destinationHolder.GetDestination(x,y,z);
            return true;               
        }
    private:
        TimeTrackerSmall i_nextMoveTime;

        DestinationHolder< Traveller<T> > i_destinationHolder;
        uint32 i_nextMove;
};


template<class T>
class MANGOS_DLL_SPEC RandomCircleMovementGenerator
: public MovementGeneratorMedium< T, RandomCircleMovementGenerator<T> >
{
    public:
        explicit RandomCircleMovementGenerator(const Unit &) : i_nextMoveTime(0) {}

        void fillSplineWayPoints(T &);
        void Initialize(T &);
        void Finalize(T &);
        void Interrupt(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);
        MovementGeneratorType GetMovementGeneratorType() const { return RANDOM_CIRCLE_MOTION_TYPE; }

        bool GetResetPosition(T&, float& x, float& y, float& z);
        uint32 GetCurrentWp() const { return i_wpId; };
        SplineWayPointMap *GetSplineMap() { return &m_splineMap; };
    private:
        TimeTrackerSmall i_nextMoveTime;

        uint32 i_wpId;
        SplineWayPointMap m_splineMap;
        bool m_bClockWise;
};

#endif
