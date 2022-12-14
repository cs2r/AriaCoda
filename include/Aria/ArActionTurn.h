/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016-2018 Omron Adept Technologies, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*/
#ifndef ARACTIONTURN
#define ARACTIONTURN

#include "Aria/ariaTypedefs.h"
#include "Aria/ArAction.h"

/// Action to turn when the behaviors with more priority have limited the speed
/**
   This action is basically made so that you can just have a ton of
   limiters of different kinds and types to keep speed under control,
   then throw this into the mix to have the robot wander.  Note that
   the turn amount ramps up to turnAmount starting at 0 at
   speedStartTurn and hitting the full amount at speedFullTurn.

   @ingroup ActionClasses
**/
class ArActionTurn : public ArAction
{
public:
  /// Constructor
  AREXPORT ArActionTurn(const char *name = "turn",
			double speedStartTurn = 200,
			double speedFullTurn = 100,
			double turnAmount = 15);
  //AREXPORT virtual ~ArActionTurn();
  AREXPORT virtual ArActionDesired *fire(ArActionDesired currentDesired) override;
  AREXPORT virtual ArActionDesired *getDesired() override { return &myDesired; }
#ifndef SWIG
  AREXPORT virtual const ArActionDesired *getDesired() const override 
                                                        { return &myDesired; }
#endif
protected:
  double mySpeedStart;
  double mySpeedFull;
  double myTurnAmount;
  double myTurning;

  ArActionDesired myDesired;

};

#endif // ARACTIONTURN
