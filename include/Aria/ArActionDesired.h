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
#ifndef ARACTIONDESIRED_H
#define ARACTIONDESIRED_H

#include "Aria/ariaTypedefs.h"
#include "Aria/ariaUtil.h"
#include <cmath>
#include <algorithm>

/// Class used by ArActionDesired for each channel, internal
class ArActionDesiredChannel
{
public:
  AREXPORT static const double NO_STRENGTH;
  AREXPORT static const double MIN_STRENGTH;
  AREXPORT static const double MAX_STRENGTH;
  
  ArActionDesiredChannel() { reset(); myOverrideDoesLessThan = true; }
  //~ArActionDesiredChannel() {}
  void setOverrideDoesLessThan(bool overrideDoesLessThan) 
    { myOverrideDoesLessThan = overrideDoesLessThan; }
  void setDesired(double desired, double desiredStrength, 
		  bool allowOverride = false) 
    {
      myDesired = desired;
      myStrength = desiredStrength; 
      myAllowOverride = allowOverride;
      if (myStrength > MAX_STRENGTH)
	myStrength = MAX_STRENGTH;
      if (myStrength < MIN_STRENGTH)
	myStrength = NO_STRENGTH;
    }
  double getDesired() const { return myDesired; }
  double getStrength() const { return myStrength; }
  bool getAllowOverride() const { return myAllowOverride; }
  void reset() 
    { myDesired = 0; myStrength = NO_STRENGTH; myAllowOverride = true; }
  void merge(ArActionDesiredChannel *desiredChannel)
    {
      double otherStrength = desiredChannel->getStrength();
      double oldStrength = myStrength;
      if (myStrength + otherStrength > MAX_STRENGTH)
	otherStrength = MAX_STRENGTH - myStrength;
      myStrength = myStrength + otherStrength;
      myAllowOverride = myAllowOverride && desiredChannel->getAllowOverride();
      // if we're allowing override just set myDesired to the least
      // (or greatest) value
      if (myAllowOverride && myStrength >= MIN_STRENGTH)
      {
	// if both have strength get the min/max
	if (oldStrength >= MIN_STRENGTH && 
	    desiredChannel->getStrength() >= MIN_STRENGTH)
	{
	  if (myOverrideDoesLessThan)
	    myDesired = std::min(myDesired, 
					desiredChannel->getDesired());
	  else //if (!myOverrideDoesLessThan)
	    myDesired = std::max(myDesired, 
					desiredChannel->getDesired());
	}
	// if only it has strength use it
	else if (desiredChannel->getStrength() >= MIN_STRENGTH)
	{
	  myDesired = desiredChannel->getDesired();
	}
	// if only this has strength then we don't need to do anything
      }
      else if (myStrength >= MIN_STRENGTH)
	myDesired = (((oldStrength * myDesired) + 
		      (desiredChannel->getDesired() * otherStrength)) 
		     / (myStrength));
    }
  void startAverage()
    {
      myDesiredTotal = myDesired * myStrength;
      myStrengthTotal = myStrength;
    }
  void addAverage(ArActionDesiredChannel *desiredChannel)
    {
      myAllowOverride = myAllowOverride && desiredChannel->getAllowOverride();
      // if we're allowing override then myDesired is just the least
      // of the values thats going to come through... still compute
      // the old way in case something doesn't want to override it
      if (myAllowOverride)
      {
	// if both have strength get the min/max
	if (myStrength >= MIN_STRENGTH && 
	    desiredChannel->getStrength() >= MIN_STRENGTH)
	{
	  if (myOverrideDoesLessThan)
	    myDesired = std::min(myDesired, 
					desiredChannel->getDesired());
	  else //if (!myOverrideDoesLessThan)
	    myDesired = std::max(myDesired, 
					desiredChannel->getDesired());
	}
	// if only it has strength use it
	else if (desiredChannel->getStrength() >= MIN_STRENGTH)
	{
	  myDesired = desiredChannel->getDesired();
	}
	// if only this has strength then we don't need to do anything
      }
      myDesiredTotal += (desiredChannel->getDesired() * 
			 desiredChannel->getStrength());
      myStrengthTotal += desiredChannel->getStrength();
    }
  void endAverage()
    {
      if (myStrengthTotal < MIN_STRENGTH)
      {
	myStrength = NO_STRENGTH;
	return;
      }
      // if we're overriding we just use what myDesired already is
      if (!myAllowOverride)
	myDesired = (myDesiredTotal / myStrengthTotal);
      myStrength = myStrengthTotal;
      if (myStrength > MAX_STRENGTH)
	myStrength = MAX_STRENGTH;
    }
  /// do some bounds checking
  void checkLowerBound(const char *actionName, const char *typeName, 
				int lowerBound)
    {
      // if it has no strength, just return
      if (myStrength < MIN_STRENGTH)
	return;
      
      if (ArMath::roundInt(myDesired) < lowerBound)
      {
	ArLog::log(ArLog::Terse, 
		   "ActionSanityChecking: '%s' tried to set %s to %g (which wound wind up less than %d and will be set to %d)",
		   actionName, typeName, myDesired, lowerBound, lowerBound);
	myDesired = lowerBound;
	return;
      }
    }
  /// do some bounds checking
  void checkUpperBound(const char *actionName, const char *typeName, 
				int upperBound)
    {
      // if it has no strength, just return
      if (myStrength < MIN_STRENGTH)
	return;
      
      if (ArMath::roundInt(myDesired) > upperBound)
      {
	ArLog::log(ArLog::Terse, 
		   "ActionSanityChecking: '%s' tried to set %s to %g (which would wind up greater than %d and will be set to %d)",
		   actionName, typeName, myDesired, upperBound, upperBound);
	myDesired = upperBound;
	return;
      }
    }


protected:
  double myDesired = 0;
  double myStrength = NO_STRENGTH;
  bool myAllowOverride = true;
  double myDesiredTotal = 0;
  double myStrengthTotal = NO_STRENGTH;
  bool myOverrideDoesLessThan = true;
};

/// Contains values returned by ArAction objects expressing desired motion commands to resolver
/**
   This class is use by actions to report what want movement commands they want. 
   The action resolver combines the ArActionDesired objects returned by different actions.

   A brief summary follows. For a fuller explanation of actions, see @ref actions.

   Different values are organized into different "channels". 
   Translational (front/back) and rotational (right/left) movements are separate 
   channels.  Translational movement uses velocity, while rotational movement uses 
   change in heading from current heading. 
   Each channel has a strength value.  
   Both translational and rotational movement have maximum velocities as well,
   that also have their own strengths.

   The strength value reflects how strongly an action wants to do the chosen 
   movement command, the resolver (ArResolver) will combine these strengths 
   and figure out what to do based on them.

   For all strength values there is a total of 1.0 combined strength avaliable.
   The range for strength is from 0 to 1.  This is simply a convention that 
   ARIA uses by default, if you don't like it, you could override this
   class the ArResolver class.

   Note that for the different maximum/accel/decel values they take an
   additional argument of whether just to use the slowest speed,
   slowest accel, or fastest decel.  By default these will just use
   safer values (slowest speed, slowest accel, fastest decel)... you
   can specify false on these for the old behavior.  Note that if
   you're safest values then the strength is largely ignored though it
   is still tracked and must still be greater than MIN_STRENGTH to
   work and it is still capped at MAX_STRENGTH).

   @sa @ref actions

*/
class ArActionDesired
{
public:
  AREXPORT static const double NO_STRENGTH;
  AREXPORT static const double MIN_STRENGTH;
  AREXPORT static const double MAX_STRENGTH;
  /// Constructor
  ArActionDesired() : myHeading(0), myHeadingStrength(MIN_STRENGTH), myHeadingSet(false) 
    {
      myTransDecelDes.setOverrideDoesLessThan(false); 
      myRotDecelDes.setOverrideDoesLessThan(false);
      myMaxNegVelDes.setOverrideDoesLessThan(false); 
    }
  // Destructor
    //virtual ~ArActionDesired() = default;

    /// Sets the velocity (mm/sec) and strength
    /**
     @param vel desired vel (mm/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  */
    void setVel(double vel, double strength = MAX_STRENGTH)
    { myVelDes.setDesired(vel, strength); }
  /// Sets the delta heading (deg) and strength
  /**
     If there's already a rotVel set this WILL NOT work.
     @param deltaHeading desired change in heading (deg)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  */
  void setDeltaHeading(double deltaHeading, 
			       double strength = MAX_STRENGTH)
    { myDeltaHeadingDes.setDesired(deltaHeading, strength); }
  /// Sets the absolute heading (deg) 
  /**
     If there's already a rotVel set this WILL NOT work.
     This is a way to set the heading instead of using a delta, there is no
     get for this, because accountForRobotHeading MUST be called (this should
     be called by all resolvers, but if you want to call it you can,
     thats fine).
     @param heading desired heading (deg)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  */
  void setHeading(double heading, double strength = MAX_STRENGTH)
    { myHeading = heading; myHeadingStrength = strength; myHeadingSet = true; }

  /// Sets the rotational velocity
  /**
     If there's already a delta heading or heading this WILL NOT work.
     @param rotVel desired rotational velocity (deg/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  **/
  void setRotVel(double rotVel, double strength = MAX_STRENGTH)
    { myRotVelDes.setDesired(rotVel, strength); }

  /// Sets the maximum velocity (+mm/sec) and strength
  /**
     This sets the maximum positive velocity for this cycle.  Check
     the ArRobot class notes for more details.
     
     @param maxVel desired maximum velocity (+mm/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH
     (1.0)
     @param useSlowest if this is true (the default) everywhere
     then the slowest maximum vel is what will be selected
  **/
  void setMaxVel(double maxVel, double strength = MAX_STRENGTH,
			 bool useSlowest = true)
    { myMaxVelDes.setDesired(maxVel, strength, useSlowest); }
  /// Sets the maximum velocity for going backwards (-mm/sec) and strength
  /**
     This sets the maximum negative velocity for this cycle.  Check
     the ArRobot class notes for more details.

     @param maxVel desired maximum velocity for going backwards (-mm/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
     @param useSlowest if this is true (the default) everywhere
     then the slowest max neg vel is what will be selected
  **/
  void setMaxNegVel(double maxVel, double strength = MAX_STRENGTH,
			    bool useSlowest = true)
    { myMaxNegVelDes.setDesired(maxVel, strength, useSlowest); }

  /// Sets the translation acceleration (deg/sec/sec) and strength
  /**
     This sets the translation acceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param transAccel desired translation acceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere then
     the slowest accel is what will be selected
  **/
  void setTransAccel(double transAccel, 
			     double strength = MAX_STRENGTH,
			     bool useSlowest = true)
    { myTransAccelDes.setDesired(transAccel, strength, useSlowest);  }

  /// Sets the translation deceleration (deg/sec/sec) and strength
  /**
     This sets the translation deceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param transDecel desired translation deceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useFastestDecel if this is true (the default) everywhere
     then the fastest decel is what will be selected
  **/
  void setTransDecel(double transDecel, double strength = MAX_STRENGTH,
			     bool useFastestDecel = true)
    { myTransDecelDes.setDesired(transDecel, strength, useFastestDecel);  }

  /// Sets the maximum rotational velocity (deg/sec) and strength
  /**
     This sets the maximum rotational velocity for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param maxVel desired maximum rotational velocity (deg/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere
     then the slowest rot vel is what will be selected
  **/
  void setMaxRotVel(double maxVel, double strength = MAX_STRENGTH,
			    bool useSlowest = true)
    { myMaxRotVelDes.setDesired(maxVel, strength, useSlowest); }

  /// Sets the maximum rotational velocity (deg/sec) in the positive direction and strength
  /**
     This sets the maximum rotational velocity for this cycle (this is
     sent down to the robot) in the positive direction.  If the
     setMaxRotVel is set to less than this that will be used instead.
     Check the ArRobot class notes for more details.

     @param maxVel desired maximum rotational velocity in the positive
     direction (deg/sec)

     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 

     @param useSlowest if this is true (the default) everywhere
     then the slowest rot vel is what will be selected
  **/
  void setMaxRotVelPos(double maxVel, double strength = MAX_STRENGTH,
			       bool useSlowest = true)
    { myMaxRotVelPosDes.setDesired(maxVel, strength, useSlowest); }

  /// Sets the maximum rotational velocity (deg/sec) in the negative direction and strength
  /**
     This sets the maximum rotational velocity for this cycle (this is
     sent down to the robot) in the negative direction.  If the
     setMaxRotVel is set to less than this that will be used instead.
     Check the ArRobot class notes for more details.

     @param maxVel desired maximum rotational velocity in the negative
     direction (deg/sec)

     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 

     @param useSlowest if this is true (the default) everywhere
     then the slowest rot vel is what will be selected
  **/
  void setMaxRotVelNeg(double maxVel, double strength = MAX_STRENGTH,
			       bool useSlowest = true)
    { myMaxRotVelNegDes.setDesired(maxVel, strength, useSlowest); }

  /// Sets the rotational acceleration (deg/sec/sec) and strength
  /**
     This sets the rotational acceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param rotAccel desired rotational acceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere
     then the slowest rot accel is what will be selected
  **/
  void setRotAccel(double rotAccel, double strength = MAX_STRENGTH,
			   bool useSlowest = true)
    { myRotAccelDes.setDesired(rotAccel, strength, useSlowest);  }

  /// Sets the rotational deceleration (deg/sec/sec) and strength
  /**
     This sets the rotational deceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param rotDecel desired rotational deceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useFastest if this is true (the default) everywhere
     then the fastest rot decel is what will be selected
  **/
  void setRotDecel(double rotDecel, double strength = MAX_STRENGTH,
			   bool useFastest = true)
    { myRotDecelDes.setDesired(rotDecel, strength, useFastest);  }

  /// Sets the left lateral velocity (mm/sec) and strength
  /**
     Note that there is only one actual velocity for lat vel, but
     instead of making people remember which way is left and right
     there are two functions, setLeftLatVel and setRightLatVel... all
     setRightLatVel does is flip the direction on the vel.  You can
     set a negative left lat vel and thats the same as setting a
     positive right vel.  You can do the same with setting a negative
     right vel to get a positive left vel.

     @param latVel desired vel (mm/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  */
  void setLeftLatVel(double latVel, double strength = MAX_STRENGTH)
    { myLatVelDes.setDesired(latVel, strength); }
  /// Sets the right lateral velocity (mm/sec) and strength
  /**
     Note that there is only one actual velocity for lat vel, but
     instead of making people remember which way is left and right
     there are two functions, setLeftLatVel and setRightLatVel... all
     setRightLatVel does is flip the direction on the vel.  You can
     set a negative left lat vel and thats the same as setting a
     positive right vel.  You can do the same with setting a negative
     right vel to get a positive left vel.

     @param latVel desired vel (mm/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0)
  */
  void setRightLatVel(double latVel, double strength = MAX_STRENGTH)
    { myLatVelDes.setDesired(-latVel, strength); }
  /// Sets the maximum lateral velocity (deg/sec) and strength
  /**
     This sets the maximum lateral velocity for this cycle.  Check
     the ArRobot class notes for more details.

     @param maxVel desired maximum lateral velocity (deg/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere
     then the slowest lat vel is what will be selected
  **/
  void setMaxLeftLatVel(double maxVel, double strength = MAX_STRENGTH,
			    bool useSlowest = true)
    { myMaxLeftLatVelDes.setDesired(maxVel, strength, useSlowest); }
  /// Sets the maximum lateral velocity (deg/sec) and strength
  /**
     This sets the maximum lateral velocity for this cycle.  Check
     the ArRobot class notes for more details.

     @param maxVel desired maximum lateral velocity (deg/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere
     then the slowest lat vel is what will be selected
  **/
  void setMaxRightLatVel(double maxVel, double strength = MAX_STRENGTH,
			    bool useSlowest = true)
    { myMaxRightLatVelDes.setDesired(maxVel, strength, useSlowest); }

  /// Sets the lateral acceleration (deg/sec/sec) and strength
  /**
     This sets the lateral acceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param latAccel desired lateral acceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useSlowest if this is true (the default) everywhere
     then the slowest lat accel is what will be selected
  **/
  void setLatAccel(double latAccel, double strength = MAX_STRENGTH,
			   bool useSlowest = true)
    { myLatAccelDes.setDesired(latAccel, strength, useSlowest);  }

  /// Sets the lateral deceleration (deg/sec/sec) and strength
  /**
     This sets the lateral deceleration for this cycle (this is
     sent down to the robot).  Check the ArRobot class notes for more
     details.

     @param latDecel desired lateral deceleration (deg/sec/sec)
     @param strength strength given to this, defaults to MAX_STRENGTH (1.0) 
     @param useFastest if this is true (the default) everywhere
     then the fastest lat decel is what will be selected
  **/
  void setLatDecel(double latDecel, double strength = MAX_STRENGTH,
			   bool useFastest = true)
    { myLatDecelDes.setDesired(latDecel, strength, useFastest);  }

  /// Resets the strengths to 0
  void reset() 
    {
      myVelDes.reset(); 
      myMaxVelDes.reset(); myMaxNegVelDes.reset(); 
      myTransAccelDes.reset(); myTransDecelDes.reset();

      myRotVelDes.reset(); myDeltaHeadingDes.reset(); 
      myMaxRotVelDes.reset(); 
      myMaxRotVelPosDes.reset(); myMaxRotVelNegDes.reset(); 
      myRotAccelDes.reset(); myRotDecelDes.reset();
      myHeadingStrength = 0;
      myHeadingSet = false;

      myLatVelDes.reset();
      myMaxLeftLatVelDes.reset();  myMaxRightLatVelDes.reset(); 
      myLatAccelDes.reset(); myLatDecelDes.reset();
    }

  /// Gets the translational velocity desired (mm/sec)
  double getVel() const
    { return myVelDes.getDesired(); }
  /// Gets the strength of the translational velocity desired
  double getVelStrength() const
    { return myVelDes.getStrength(); }
  /// Gets the heading desired (deg)
  double getHeading() const
    { return myHeading; }
  /// Gets the strength of the heading desired
  double getHeadingStrength() const
    { return myHeadingStrength; }
  /// Gets the delta heading desired (deg)
  double getDeltaHeading() const
    { return myDeltaHeadingDes.getDesired(); }
  /// Gets the strength of the delta heading desired
  double getDeltaHeadingStrength() const
    { return myDeltaHeadingDes.getStrength(); }
  /// Gets the rot vel that was set
  double getRotVel() const { return myRotVelDes.getDesired(); }
  /// Gets the rot vel des (deg/sec)
  double getRotVelStrength() const 
    { return myRotVelDes.getStrength(); }

  /// Gets the desired maximum velocity (mm/sec)
  double getMaxVel() const
    { return myMaxVelDes.getDesired(); }
  /// Gets the maximum velocity strength
  double getMaxVelStrength() const
    { return myMaxVelDes.getStrength(); }
  /// Gets whether the slowest is being used or not
  double getMaxVelSlowestUsed() const
    { return myMaxVelDes.getAllowOverride(); }
  /// Gets the desired maximum negative velocity (-mm/sec)
  double getMaxNegVel() const
    { return myMaxNegVelDes.getDesired(); }
  /// Gets the desired maximum negative velocity strength
  double getMaxNegVelStrength() const
    { return myMaxNegVelDes.getStrength(); }
  /// Gets whether the slowest is being used or not
  double getMaxNegVelSlowestUsed() const
    { return myMaxNegVelDes.getAllowOverride(); }
  /// Gets the desired trans acceleration (mm/sec)
  double getTransAccel() const
    { return myTransAccelDes.getDesired(); }
  /// Gets the desired trans acceleration strength
  double getTransAccelStrength() const
    { return myTransAccelDes.getStrength(); }
  /// Gets whether the slowest accel is being used or not
  double getTransAccelSlowestUsed() const
    { return myTransAccelDes.getAllowOverride(); }
  /// Gets the desired trans deceleration (-mm/sec/sec)
  double getTransDecel() const
    { return myTransDecelDes.getDesired(); }
  /// Gets the desired trans deceleration strength
  double getTransDecelStrength() const
    { return myTransDecelDes.getStrength(); }
  /// Gets whether the fastest decel is being used or not
  double getTransDecelFastestUsed() const
    { return myTransDecelDes.getAllowOverride(); }

  /// Gets the maximum rotational velocity
  double getMaxRotVel() const
    { return myMaxRotVelDes.getDesired(); }
  /// Gets the maximum rotational velocity strength
  double getMaxRotVelStrength() const
    { return myMaxRotVelDes.getStrength(); }
  /// Gets whether the slowest rot vel is being used or not
  double getMaxRotVelSlowestUsed() const
    { return myMaxRotVelDes.getAllowOverride(); }

  /// Gets the maximum rotational velocity in the positive direction
  double getMaxRotVelPos() const
    { return myMaxRotVelPosDes.getDesired(); }
  /// Gets the maximum rotational velocity in the positive direction strength
  double getMaxRotVelPosStrength() const
    { return myMaxRotVelPosDes.getStrength(); }
  /// Gets whether the slowest rot vel in the positive direction is being used or not
  double getMaxRotVelPosSlowestUsed() const
    { return myMaxRotVelPosDes.getAllowOverride(); }

  /// Gets the maximum rotational velocity in the negative direction
  double getMaxRotVelNeg() const
    { return myMaxRotVelNegDes.getDesired(); }
  /// Gets the maximum rotational velocity in the negative direction strength
  double getMaxRotVelNegStrength() const
    { return myMaxRotVelNegDes.getStrength(); }
  /// Gets whether the slowest rot vel in the negative direction is being used or not
  double getMaxRotVelNegSlowestUsed() const
    { return myMaxRotVelNegDes.getAllowOverride(); }

  /// Gets the desired rotational acceleration (mm/sec)
  double getRotAccel() const
    { return myRotAccelDes.getDesired(); }
  /// Gets the desired rotational acceleration strength
  double getRotAccelStrength() const
    { return myRotAccelDes.getStrength(); }
  /// Gets whether the slowest rot accel is being used or not
  double getRotAccelSlowestUsed() const
    { return myRotAccelDes.getAllowOverride(); }
  /// Gets the desired rotational deceleration (-mm/sec/sec)
  double getRotDecel() const
    { return myRotDecelDes.getDesired(); }
  /// Gets the desired rotational deceleration strength
  double getRotDecelStrength() const
    { return myRotDecelDes.getStrength(); }
  /// Gets whether the fastest rot decel is being used or not
  double getRotDecelFastestUsed() const
    { return myRotDecelDes.getAllowOverride(); }

  /// Gets the lat vel that was set
  double getLatVel() const { return myLatVelDes.getDesired(); }
  /// Gets the lat vel des (deg/sec)
  double getLatVelStrength() const 
    { return myLatVelDes.getStrength(); }
  /// Gets the maximum lateral velocity
  double getMaxLeftLatVel() const
    { return myMaxLeftLatVelDes.getDesired(); }
  /// Gets the maximum lateral velocity strength
  double getMaxLeftLatVelStrength() const
    { return myMaxLeftLatVelDes.getStrength(); }
  /// Gets whether the slowest lat vel is being used or not
  double getMaxLeftLatVelSlowestUsed() const
    { return myMaxLeftLatVelDes.getAllowOverride(); }
  /// Gets the maximum lateral velocity
  double getMaxRightLatVel() const
    { return myMaxRightLatVelDes.getDesired(); }
  /// Gets the maximum lateral velocity strength
  double getMaxRightLatVelStrength() const
    { return myMaxRightLatVelDes.getStrength(); }
  /// Gets whether the slowest lat vel is being used or not
  double getMaxRightLatVelSlowestUsed() const
    { return myMaxRightLatVelDes.getAllowOverride(); }
  /// Gets the desired lateral acceleration (mm/sec)
  double getLatAccel() const
    { return myLatAccelDes.getDesired(); }
  /// Gets the desired lateral acceleration strength
  double getLatAccelStrength() const
    { return myLatAccelDes.getStrength(); }
  /// Gets whether the slowest lat accel is being used or not
  double getLatAccelSlowestUsed() const
    { return myLatAccelDes.getAllowOverride(); }
  /// Gets the desired lateral deceleration (-mm/sec/sec)
  double getLatDecel() const
    { return myLatDecelDes.getDesired(); }
  /// Gets the desired lateral deceleration strength
  double getLatDecelStrength() const
    { return myLatDecelDes.getStrength(); }
  /// Gets whether the fastest lat decel is being used or not
  double getLatDecelFastestUsed() const
    { return myLatDecelDes.getAllowOverride(); }


  /// Merges the given ArActionDesired into this one (this one has precedence),
  /// internal
  /** 
      This merges in the two different action values, accountForRobotHeading
      MUST be done before this is called (on both actions), since this merges
      their delta headings, and the deltas can't be known unless the account
      for angle is done.
      @param actDesired the actionDesired to merge with this one
  */
  void merge(ArActionDesired *actDesired)
    {
      if (actDesired == NULL)
	return;
      myVelDes.merge(&actDesired->myVelDes);
      // if we're already using rot or delt use that, otherwise use what it wants
      if (myDeltaHeadingDes.getStrength() > NO_STRENGTH)
      {
	myDeltaHeadingDes.merge(&actDesired->myDeltaHeadingDes);
      }
      else if (myRotVelDes.getStrength() > NO_STRENGTH)
      {
	myRotVelDes.merge(&actDesired->myRotVelDes);
      }
      else
      {
	myDeltaHeadingDes.merge(&actDesired->myDeltaHeadingDes);
	myRotVelDes.merge(&actDesired->myRotVelDes);
      }
      myMaxVelDes.merge(&actDesired->myMaxVelDes);
      myMaxNegVelDes.merge(&actDesired->myMaxNegVelDes);
      myMaxRotVelDes.merge(&actDesired->myMaxRotVelDes);
      myMaxRotVelPosDes.merge(&actDesired->myMaxRotVelPosDes);
      myMaxRotVelNegDes.merge(&actDesired->myMaxRotVelNegDes);
      myTransAccelDes.merge(&actDesired->myTransAccelDes);
      myTransDecelDes.merge(&actDesired->myTransDecelDes);
      myRotAccelDes.merge(&actDesired->myRotAccelDes);
      myRotDecelDes.merge(&actDesired->myRotDecelDes);

      myLatVelDes.merge(&actDesired->myLatVelDes);
      myMaxLeftLatVelDes.merge(&actDesired->myMaxLeftLatVelDes);
      myMaxRightLatVelDes.merge(&actDesired->myMaxRightLatVelDes);
      myLatAccelDes.merge(&actDesired->myLatAccelDes);
      myLatDecelDes.merge(&actDesired->myLatDecelDes);
    }
  /// Starts the process of averaging together different desired action changes
  /**
     There is a three step process for averaging desired actions together,
     first startAverage must be done to set up the process, then addAverage
     must be done with each average that is desired, then finally endAverage
     should be used, after that is done then the normal process of getting
     the results out should be done.
  */
  void startAverage()
    {
      myVelDes.startAverage();
      myMaxVelDes.startAverage();
      myMaxNegVelDes.startAverage();
      myTransAccelDes.startAverage();
      myTransDecelDes.startAverage();


      myRotVelDes.startAverage();
      myDeltaHeadingDes.startAverage();
      myMaxRotVelDes.startAverage();
      myMaxRotVelPosDes.startAverage();
      myMaxRotVelNegDes.startAverage();
      myRotAccelDes.startAverage();
      myRotDecelDes.startAverage();

      myLatVelDes.startAverage();
      myMaxLeftLatVelDes.startAverage();
      myMaxRightLatVelDes.startAverage();
      myLatAccelDes.startAverage();
      myLatDecelDes.startAverage();
    }
  /// Adds another actionDesired into the mix to average
  /**
     For a description of how to use this, see startAverage.
     @param actDesired the actionDesired to add into the average
  */
  void addAverage(ArActionDesired *actDesired)
    {
      if (actDesired == NULL)
	return;
      myVelDes.addAverage(&actDesired->myVelDes);

      myMaxVelDes.addAverage(&actDesired->myMaxVelDes);
      myMaxNegVelDes.addAverage(&actDesired->myMaxNegVelDes);
      myTransAccelDes.addAverage(&actDesired->myTransAccelDes);
      myTransDecelDes.addAverage(&actDesired->myTransDecelDes);

      // if we're using one of rot or delta heading use that,
      // otherwise use whatever they're using
      if (myRotVelDes.getStrength() > NO_STRENGTH)
      {
	myRotVelDes.addAverage(
		&actDesired->myRotVelDes);
      }
      else if (myDeltaHeadingDes.getStrength() > NO_STRENGTH)
      {
	myDeltaHeadingDes.addAverage(
		&actDesired->myDeltaHeadingDes);
      }
      else
      {
	myRotVelDes.addAverage(
		&actDesired->myRotVelDes);
	myDeltaHeadingDes.addAverage(
		&actDesired->myDeltaHeadingDes);
      }
      myMaxRotVelDes.addAverage(&actDesired->myMaxRotVelDes);
      myMaxRotVelPosDes.addAverage(&actDesired->myMaxRotVelPosDes);
      myMaxRotVelNegDes.addAverage(&actDesired->myMaxRotVelNegDes);
      myRotAccelDes.addAverage(&actDesired->myRotAccelDes);
      myRotDecelDes.addAverage(&actDesired->myRotDecelDes);

      myLatVelDes.addAverage(&actDesired->myLatVelDes);
      myMaxLeftLatVelDes.addAverage(&actDesired->myMaxLeftLatVelDes);
      myMaxRightLatVelDes.addAverage(&actDesired->myMaxRightLatVelDes);
      myLatAccelDes.addAverage(&actDesired->myLatAccelDes);
      myLatDecelDes.addAverage(&actDesired->myLatDecelDes);
    }
  /// Ends the process of averaging together different desired actions
  /**
     For a description of how to use this, see startAverage.
  */
  void endAverage()
    {
      myVelDes.endAverage();
      myMaxVelDes.endAverage();
      myMaxNegVelDes.endAverage();
      myTransAccelDes.endAverage();
      myTransDecelDes.endAverage();

      myRotVelDes.endAverage();
      myDeltaHeadingDes.endAverage();
      myMaxRotVelDes.endAverage();
      myMaxRotVelPosDes.endAverage();
      myMaxRotVelNegDes.endAverage();
      myRotAccelDes.endAverage();
      myRotDecelDes.endAverage();

      myLatVelDes.endAverage();
      myMaxLeftLatVelDes.endAverage();
      myMaxRightLatVelDes.endAverage();
      myLatAccelDes.endAverage();
      myLatDecelDes.endAverage();
    }
  /// Accounts for robot heading, mostly internal
  /**
     This accounts for the robots heading, and transforms the set heading
     on this actionDesired into a delta heading so it can be merged and 
     averaged and the like
     @param robotHeading the heading the real actual robot is at now
   */
  void accountForRobotHeading(double robotHeading)
    {
      if (myHeadingSet)
	setDeltaHeading(ArMath::subAngle(myHeading, robotHeading), 
			myHeadingStrength);
      myHeadingSet = false;
    }
  /// Logs what is desired
  AREXPORT void log() const;
  /// Gets whether anything is desired (should only really be used in relation to logging)
  AREXPORT bool isAnythingDesired() const;
  /// Do a sanity check on the action (this is set up this way so the
  /// action name can be passed in)
  AREXPORT void sanityCheck(const char *actionName);

protected:
  double myHeading;
  double myHeadingStrength;
  bool myHeadingSet;

  ArActionDesiredChannel myVelDes;
  ArActionDesiredChannel myMaxVelDes;
  ArActionDesiredChannel myMaxNegVelDes;
  ArActionDesiredChannel myTransAccelDes;
  ArActionDesiredChannel myTransDecelDes;

  ArActionDesiredChannel myRotVelDes;
  ArActionDesiredChannel myDeltaHeadingDes;
  ArActionDesiredChannel myMaxRotVelDes;
  ArActionDesiredChannel myMaxRotVelPosDes;
  ArActionDesiredChannel myMaxRotVelNegDes;
  ArActionDesiredChannel myRotAccelDes;
  ArActionDesiredChannel myRotDecelDes;

  ArActionDesiredChannel myLatVelDes;
  ArActionDesiredChannel myMaxLeftLatVelDes;
  ArActionDesiredChannel myMaxRightLatVelDes;
  ArActionDesiredChannel myLatAccelDes;
  ArActionDesiredChannel myLatDecelDes;
};


#endif
