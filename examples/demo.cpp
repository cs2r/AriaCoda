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

#include "Aria/Aria.h"
#include "Aria/ariaTypedefs.h"
#include "Aria/ArActionGroups.h"
#include "Aria/ArGripper.h"
#include "Aria/ArTcpConnection.h"
#include "Aria/ArSerialConnection.h"
#include "Aria/ArPTZ.h"
#include "Aria/ArRobotConfigPacketReader.h"
#include "Aria/ArFunctor.h"
#include "Aria/ArExport.h"
#include "Aria/ariaOSDef.h"
#include "Aria/ArRobot.h"
#include "Aria/ariaInternal.h"
#include "Aria/ArKeyHandler.h"
//#include "Aria/ArSonyPTZ.h"
#include "Aria/ArVCC4.h"
#include "Aria/ArDPPTU.h"
//#include "Aria/ArAMPTU.h"
#include "Aria/ArRVisionPTZ.h"
//#include "Aria/ArSick.h"
#include "Aria/ArLaser.h"
#include "Aria/ArAnalogGyro.h"
#include "Aria/ArRobotConfigPacketReader.h"
#include "Aria/ArBatteryMTX.h"
#include "Aria/ArSimUtil.h"

#include <string>
#include <list>
#include <sstream>
#include <iomanip>
#include <ios>


//const size_t OutputBuffLen = 1056;

/** @example demo.cpp General purpose testing and demo program, using ArMode
 *    classes to provide keyboard control of various robot functions.
 *
 *  demo uses ArMode subclasses from ARIA. These modes
 *  provide keyboard control of various aspects and accessories of 
 *  the robot, and can be re-used in your programs if you wish.  
 *  The ArMode classes are defined in %ArModes.cpp.
 *
 *  "demo" is a useful program for testing out the operation of the robot
 *  for diagnostic or demonstration purposes.  Other example programs
 *  focus on individual areas.
 */

bool handleDebugMessage(ArRobotPacket *pkt)
{
  if(pkt->getID() != ArCommands::MARCDEBUG) return false;
  char msg[256];
  pkt->bufToStr(msg, sizeof(msg));
  msg[255] = 0;
  ArLog::log(ArLog::Terse, "Controller Firmware Debug: %s", msg);
  return true;
}





/// A class for different modes, mostly as related to keyboard input
/**
  Each mode is going to need to add its keys to the keyHandler...
  each mode should only use the keys 1-0, the arrow keys (movement),
  the space bar (stop), z (zoom in), x (zoom out), and e (exercise)...
  then when its activate is called by that key handler it needs to
  first deactivate the ourActiveMode (if its not itself, in which case
  its done) then add its key handling stuff... activate and deactivate
  will need to add and remove their user tasks (or call the base class
  activate/deactivate to do it) as well as the key handling things for
  their other part of modes.  This mode will ALWAYS bind help to /, ?, h,
  and H when the first instance of an ArMode is made.

 @ingroup OptionalClasses
**/
class ArMode 
{
public:
  /// Constructor
  ArMode(ArRobot *robot, const char *name, char key, char key2);
  /// Destructor
  virtual ~ArMode();
  /// Gets the name of the mode
  const char *getName();
  /// The function called when the mode is activated, subclass must provide
  virtual void activate() = 0;
  /// The function called when the mode is deactivated, subclass must provide
  virtual void deactivate() = 0;
  /// The ArMode's user task, don't need one, subclass must provide if needed
  virtual void userTask() {}
  /// The mode's help print out... subclass must provide if needed
  /** 
      This is called as soon as a mode is activated, and should give
      directions on to what keys do what and what this mode will do
  **/
  virtual void help() {}
  /// The base activation, it MUST be called by inheriting classes,
  /// and inheriting classes MUST return if this returns false
  bool baseActivate(); 
  /// The base deactivation, it MUST be called by inheriting classes,
  /// and inheriting classes MUST return if this returns false
  bool baseDeactivate();
  /// This is the base help function, its internal, bound to ? and h and H
  static void baseHelp();
  /// An internal function to get the first key this is bound to
  char getKey();
  /// An internal function to get the second key this is bound to
  char getKey2();
protected:
  void addKeyHandler(int keyToHandle, ArFunctor *functor);
  void remKeyHandler(ArFunctor *functor);
  // Our activeArMode
  static ArMode *ourActiveMode;
  std::string myName;
  ArRobot *myRobot;
  ArFunctorC<ArMode> myActivateCB;
  ArFunctorC<ArMode> myDeactivateCB;
  ArFunctorC<ArMode> myUserTaskCB;
  char myKey;
  char myKey2;
  // our help callback, its NULL until its initialized
  static ArGlobalFunctor *ourHelpCB;
  static std::list<ArMode *> ourModes;
};


/// Mode for teleoping the robot with joystick + keyboard
class ArModeTeleop : public ArMode
{
public:
  /// Constructor
  ArModeTeleop(ArRobot *robot, const char *name, char key, char key2);
  /// Destructor
  virtual ~ArModeTeleop();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
protected:
  //ArActionGroupTeleop myGroup;
  // use our new ratio drive instead
  ArActionGroupRatioDrive myGroup;
  ArFunctorC<ArRobot> myEnableMotorsCB;
};

/// Mode for teleoping the robot with joystick + keyboard
class ArModeUnguardedTeleop : public ArMode
{
public:
  /// Constructor
  ArModeUnguardedTeleop(ArRobot *robot, const char *name, char key, char key2);
  /// Destructor
  virtual ~ArModeUnguardedTeleop();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
protected:
  //ArActionGroupUnguardedTeleop myGroup;
  // use our new ratio drive instead
  ArActionGroupRatioDriveUnsafe myGroup;
  ArFunctorC<ArRobot> myEnableMotorsCB;
};

/// Mode for wandering around
class ArModeWander : public ArMode
{
public:
  /// Constructor
  ArModeWander(ArRobot *robot, const char *name, char key, char key2);
  /// Destructor
  virtual ~ArModeWander();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
protected:
  ArActionGroupWander myGroup;
};

/// Mode for controlling the gripper
class ArModeGripper : public ArMode
{
public:
  /// Constructor
  ArModeGripper(ArRobot *robot, const char *name, char key,
			 char key2);
  /// Destructor
  virtual ~ArModeGripper();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
  void open();
  void close();
  void up();
  void down();
  void stop();
  void exercise();
protected:
  enum ExerState {
    UP_OPEN,
    UP_CLOSE,
    DOWN_CLOSE,
    DOWN_OPEN
  };
  ArGripper myGripper;
  bool myExercising;
  ExerState myExerState;
  ArTime myLastExer;
  ArFunctorC<ArModeGripper> myOpenCB;
  ArFunctorC<ArModeGripper> myCloseCB;
  ArFunctorC<ArModeGripper> myUpCB;
  ArFunctorC<ArModeGripper> myDownCB;
  ArFunctorC<ArModeGripper> myStopCB;
  ArFunctorC<ArModeGripper> myExerciseCB;
  
};

/// Mode for controlling the camera
class ArModeCamera : public ArMode
{
public:
  /// Constructor
  ArModeCamera(ArRobot *robot, const char *name, char key,
			 char key2);
  /// Destructor
  virtual ~ArModeCamera();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
  void up();
  void down();
  void left();
  void right();
  void center();
  void zoomIn();
  void zoomOut();
  void exercise();
  void toggleAutoFocus();
//  void sony();
  void canon();
  void dpptu();
//  void amptu();
  void canonInverted();
//  void sonySerial();
  void canonSerial();
  void dpptuSerial();
//  void amptuSerial();
  void canonInvertedSerial();
  void rvisionSerial();
  void com1();
  void com2();
  void com3();
  void com4();
  void usb0();
  void usb9();
  void aux1();
  void aux2();
protected:
  void takeCameraKeys();
  void giveUpCameraKeys();
  void helpCameraKeys();
  void takePortKeys();
  void giveUpPortKeys();
  void helpPortKeys();
  void takeAuxKeys();
  void giveUpAuxKeys();
  void helpAuxKeys();
  void takeMovementKeys();
  void giveUpMovementKeys();
  void helpMovementKeys();
  enum State {
    STATE_CAMERA,
    STATE_PORT,
    STATE_MOVEMENT
  };
  void cameraToMovement();
  void cameraToPort();
  void cameraToAux();
  void portToMovement();
  void auxToMovement();
  enum ExerState {
    CENTER,
    UP_LEFT,
    UP_RIGHT,
    DOWN_RIGHT,
    DOWN_LEFT
  };

  bool myExercising;
  State myState;
  ExerState myExerState;
  ArTime myLastExer;
  bool myExerZoomedIn;
  ArTime myLastExerZoomed;
  ArSerialConnection myConn;
  ArPTZ *myCam;
  ArFunctorC<ArModeCamera> myUpCB;
  ArFunctorC<ArModeCamera> myDownCB;
  ArFunctorC<ArModeCamera> myLeftCB;
  ArFunctorC<ArModeCamera> myRightCB;
  ArFunctorC<ArModeCamera> myCenterCB;
  ArFunctorC<ArModeCamera> myZoomInCB;
  ArFunctorC<ArModeCamera> myZoomOutCB;
  ArFunctorC<ArModeCamera> myExerciseCB;
//  ArFunctorC<ArModeCamera> mySonyCB;
  ArFunctorC<ArModeCamera> myCanonCB;
  ArFunctorC<ArModeCamera> myDpptuCB;
//  ArFunctorC<ArModeCamera> myAmptuCB;
  ArFunctorC<ArModeCamera> myCanonInvertedCB;
//  ArFunctorC<ArModeCamera> mySonySerialCB;
  ArFunctorC<ArModeCamera> myCanonSerialCB;
  ArFunctorC<ArModeCamera> myDpptuSerialCB;
//  ArFunctorC<ArModeCamera> myAmptuSerialCB;
  ArFunctorC<ArModeCamera> myCanonInvertedSerialCB;
  ArFunctorC<ArModeCamera> myRVisionSerialCB;
  ArFunctorC<ArModeCamera> myCom1CB;
  ArFunctorC<ArModeCamera> myCom2CB;
  ArFunctorC<ArModeCamera> myCom3CB;
  ArFunctorC<ArModeCamera> myCom4CB;
  ArFunctorC<ArModeCamera> myUSBCom0CB;
  ArFunctorC<ArModeCamera> myUSBCom9CB;
  ArFunctorC<ArModeCamera> myAux1CB;
  ArFunctorC<ArModeCamera> myAux2CB;
  const int myPanAmount;
  const int myTiltAmount;
  bool myAutoFocusOn;
  ArFunctorC<ArModeCamera> myToggleAutoFocusCB;
};

/// Mode for displaying the sonar
class ArModeSonar : public ArMode
{
public:
  /// Constructor
  ArModeSonar(ArRobot *robot, const char *name, char key, char key2);
  /// Destructor
  virtual ~ArModeSonar();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
  void allSonar();
  void firstSonar();
  void secondSonar();
  void thirdSonar();
  void fourthSonar();
protected:
  enum State 
  {
    STATE_ALL,
    STATE_FIRST,
    STATE_SECOND,
    STATE_THIRD,
    STATE_FOURTH
  };
  State myState;
  ArFunctorC<ArModeSonar> myAllSonarCB;
  ArFunctorC<ArModeSonar> myFirstSonarCB;
  ArFunctorC<ArModeSonar> mySecondSonarCB;
  ArFunctorC<ArModeSonar> myThirdSonarCB;
  ArFunctorC<ArModeSonar> myFourthSonarCB;
};

class ArModeBumps : public ArMode
{
public:
  ArModeBumps(ArRobot *robot, const char *name, char key, char key2);
  ~ArModeBumps();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
};

class ArModePosition : public ArMode
{
public:
  ArModePosition(ArRobot *robot, const char *name, char key,
			  char key2, ArAnalogGyro *gyro = NULL);
  ~ArModePosition();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
  void up();
  void down();
  void left();
  void right();
  void stop();
  void reset();
  void simreset();
  void mode();
  void gyro();
  void incDistance();
  void decDistance();
protected:
  enum Mode { MODE_BOTH, MODE_EITHER };
  ArAnalogGyro *myGyro;
  double myGyroZero;
  double myRobotZero;
  Mode myMode;
  std::string myModeString;
  bool myInHeadingMode;
  double myHeading;
  double myDistance;
  ArFunctorC<ArModePosition> myUpCB;
  ArFunctorC<ArModePosition> myDownCB;
  ArFunctorC<ArModePosition> myLeftCB;
  ArFunctorC<ArModePosition> myRightCB;
  ArFunctorC<ArModePosition> myStopCB;  
  ArFunctorC<ArModePosition> myResetCB;
  ArFunctorC<ArModePosition> mySimResetCB;
  ArFunctorC<ArModePosition> myModeCB;
  ArFunctorC<ArModePosition> myGyroCB;
  ArFunctorC<ArModePosition> myIncDistCB;
  ArFunctorC<ArModePosition> myDecDistCB;
};

class ArModeIO : public ArMode
{
public:
  ArModeIO(ArRobot *robot, const char *name, char key,
			  char key2);
  ~ArModeIO();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
protected:
  bool myExplanationReady = false; // flag to build myExplanation (table header) in first call to userTask() only
  bool myExplained = false;
  ArTime myLastPacketTime;
  //char myExplanation[OutputBuffLen];
  //char myOutput[OutputBuffLen];
  std::stringstream myExplanation;
  //std::stringstream myOutput;
  ArFunctorC<ArModeIO> myProcessIOCB;
  void toggleOutput(int output);
  ArFunctor1C<ArModeIO, int> myTog1CB;
  ArFunctor1C<ArModeIO, int> myTog2CB;
  ArFunctor1C<ArModeIO, int> myTog3CB;
  ArFunctor1C<ArModeIO, int> myTog4CB;
  ArFunctor1C<ArModeIO, int> myTog5CB;
  ArFunctor1C<ArModeIO, int> myTog6CB;
  ArFunctor1C<ArModeIO, int> myTog7CB;
  ArFunctor1C<ArModeIO, int> myTog8CB;
};

class ArModeLaser : public ArMode
{
public:
  ArModeLaser(ArRobot *robot, const char *name, char key, char key2);
  ~ArModeLaser();
  virtual void activate();
  virtual void deactivate();
  virtual void userTask();
  virtual void help();
  virtual void switchToLaser(int laserNumber);

protected:
  void togMiddle();
  void togConnect();

  enum State {
    STATE_UNINITED,
    STATE_CONNECTING,
    STATE_CONNECTED
  };
  
  State myState;
  ArLaser *myLaser;
  int myLaserNumber;

  bool myPrintMiddle;

  ArFunctorC<ArModeLaser> myTogMiddleCB;

  std::map<int, ArLaser *> myLasers;
  std::map<int, ArFunctor1C<ArModeLaser, int> *> myLaserCallbacks;
};

/*
/// Mode for following a color blob using ACTS
class ArModeActs : public ArMode
{
public:
  /// Constructor
  ArModeActs(ArRobot *robot, const char *name, char key, char key2,
		      ArACTS_1_2 *acts = NULL);
  /// Destructor
  virtual ~ArModeActs();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
  virtual void channel1();
  virtual void channel2();
  virtual void channel3();
  virtual void channel4();
  virtual void channel5();
  virtual void channel6();
  virtual void channel7();
  virtual void channel8();
  virtual void stop();
  virtual void start();
  virtual void toggleAcquire();
  
protected:
  ArActionGroupColorFollow *myGroup;
  ArPTZ *camera;
  ArACTS_1_2 *myActs;
  ArRobot *myRobot;

  ArFunctorC<ArModeActs> myChannel1CB;
  ArFunctorC<ArModeActs> myChannel2CB;
  ArFunctorC<ArModeActs> myChannel3CB;
  ArFunctorC<ArModeActs> myChannel4CB;
  ArFunctorC<ArModeActs> myChannel5CB;
  ArFunctorC<ArModeActs> myChannel6CB;
  ArFunctorC<ArModeActs> myChannel7CB;
  ArFunctorC<ArModeActs> myChannel8CB;
  ArFunctorC<ArModeActs> myStopCB;
  ArFunctorC<ArModeActs> myStartCB;
  ArFunctorC<ArModeActs> myToggleAcquireCB;
};
*/

class ArModeCommand : public ArMode
{
public:
  ArModeCommand(ArRobot *robot, const char *name, char key,
			  char key2);
  ~ArModeCommand();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
protected:
  void takeKeys();
  void giveUpKeys();
  void addChar(int ch);
  void finishParsing();
  void reset(bool print = true);
  char myCommandString[70];
  ArFunctor1C<ArModeCommand, int> my0CB;
  ArFunctor1C<ArModeCommand, int> my1CB;
  ArFunctor1C<ArModeCommand, int> my2CB;
  ArFunctor1C<ArModeCommand, int> my3CB;
  ArFunctor1C<ArModeCommand, int> my4CB;
  ArFunctor1C<ArModeCommand, int> my5CB;
  ArFunctor1C<ArModeCommand, int> my6CB;
  ArFunctor1C<ArModeCommand, int> my7CB;
  ArFunctor1C<ArModeCommand, int> my8CB;
  ArFunctor1C<ArModeCommand, int> my9CB;
  ArFunctor1C<ArModeCommand, int> myMinusCB;
  ArFunctor1C<ArModeCommand, int> myBackspaceCB;
  ArFunctor1C<ArModeCommand, int> mySpaceCB;
  ArFunctorC<ArModeCommand> myEnterCB;

};

/// Mode for following a color blob using ACTS
/*
class ArModeTCM2 : public ArMode
{
public:
  /// Constructor
  ArModeTCM2(ArRobot *robot, const char *name, char key, char key2, ArTCM2 *tcm2 = NULL);
  /// Destructor
  virtual ~ArModeTCM2();
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
  
protected:
  void init();
  ArTCM2 *myTCM2;
  ArCompassConnector *connector;
  ArRobot *myRobot;
  ArFunctorC<ArTCM2> *myOffCB;
  ArFunctorC<ArTCM2> *myCompassCB;
  ArFunctorC<ArTCM2> *myOnePacketCB;
  ArFunctorC<ArTCM2> *myContinuousPacketsCB;
  ArFunctorC<ArTCM2> *myUserCalibrationCB;
  ArFunctorC<ArTCM2> *myAutoCalibrationCB;
  ArFunctorC<ArTCM2> *myStopCalibrationCB;
  ArFunctorC<ArTCM2> *myResetCB;

};
*/


/// Mode for requesting config packet
class ArModeConfig : public ArMode
{
public:
  /// Constructor
  ArModeConfig(ArRobot *robot, const char *name, char key, char key2);
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  
protected:
  ArRobot *myRobot;
  ArRobotConfigPacketReader myConfigPacketReader;
  ArFunctorC<ArModeConfig> myGotConfigPacketCB;

  void gotConfigPacket();
};



/// Mode for displaying status and diagnostic info
class ArModeRobotStatus : public ArMode
{
public:
  ArModeRobotStatus(ArRobot *robot, const char *name, char key, char key2);
  virtual void activate();
  virtual void deactivate();
  virtual void help();
  virtual void userTask();
  
protected:
  ArRobot *myRobot;
  ArRetFunctor1C<bool, ArModeRobotStatus, ArRobotPacket*> myDebugMessageCB;
  ArRetFunctor1C<bool, ArModeRobotStatus, ArRobotPacket*> mySafetyStateCB;
  ArRetFunctor1C<bool, ArModeRobotStatus, ArRobotPacket*> mySafetyWarningCB;

  void printFlags();
  void printFlagsHeader();

  //std::string byte_as_bitstring(unsigned char byte);
  //std::string int16_as_bitstring(int16_t n);
  //std::string int32_as_bitstring(int32_t n);

  bool handleDebugMessage(ArRobotPacket *p);
  bool handleSafetyStatePacket(ArRobotPacket *p);
  const char *safetyStateName(int state);
  bool handleSafetyWarningPacket(ArRobotPacket *p);

  bool myBatteryShutdown;
  void toggleShutdown();
  ArFunctorC<ArModeRobotStatus> myToggleShutdownCB;
};



int main(int argc, char** argv)
{
  // Initialize some global data
  Aria::init();

  // If you want ArLog to print "Verbose" level messages uncomment this:
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  // This object parses program options from the command line
  ArArgumentParser parser(&argc, argv);

  // Load some default values for command line arguments from /etc/Aria.args
  // (Linux) or the ARIAARGS environment variable.
  parser.loadDefaultArguments();

  // Central object that is an interface to the robot and its integrated
  // devices, and which manages control of the robot by the rest of the program.
  ArRobot robot;

  // Object that connects to the robot or simulator using program options
  ArRobotConnector robotConnector(&parser, &robot);

  // If the robot has an Analog Gyro, this object will activate it, and 
  // if the robot does not automatically use the gyro to correct heading,
  // this object reads data from it and corrects the pose in ArRobot
  ArAnalogGyro gyro(&robot);

  robot.addPacketHandler(new ArGlobalRetFunctor1<bool, ArRobotPacket*>(&handleDebugMessage));

  // Connect to the robot, get some initial data from it such as type and name,
  // and then load parameter files for this robot.
  if (!robotConnector.connectRobot())
  {
    // Error connecting:
    // if the user gave the -help argumentp, then just print out what happened,
    // and continue so options can be displayed later.
    if (!parser.checkHelpAndWarnUnparsed())
    {
      ArLog::log(ArLog::Terse, "Could not connect to robot, will not have parameter file so options displayed later may not include everything");
    }
    // otherwise abort
    else
    {
      ArLog::log(ArLog::Terse, "Error, could not connect to robot.");
      Aria::logOptions();
      Aria::exit(1);
    }
  }

  if(!robot.isConnected())
  {
    ArLog::log(ArLog::Terse, "Internal error: robot connector succeeded but ArRobot::isConnected() is false!");
  }

  // Connector for laser rangefinders
  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);

  // Connector for compasses
  //ArCompassConnector compassConnector(&parser);

  // Parse the command line options. Fail and print the help message if the parsing fails
  // or if the help was requested with the -help option
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    Aria::logOptions();
    Aria::exit(1);
    return 1;
  }

  // Used to access and process sonar range data
  ArSonarDevice sonarDev;
  
  // Used to perform actions when keyboard keys are pressed
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);

  // ArRobot contains an exit action for the Escape key. It also 
  // stores a pointer to the keyhandler so that other parts of the program can
  // use the same keyhandler.
  robot.attachKeyHandler(&keyHandler);
  printf("You may press escape to exit\n");

  // Attach sonarDev to the robot so it gets data from it.
  robot.addRangeDevice(&sonarDev);

  
  // Start the robot task loop running in a new background thread. The 'true' argument means if it loses
  // connection the task loop stops and the thread exits.
  robot.runAsync(true);

  // Connect to the laser(s) if lasers were configured in this robot's parameter
  // file or on the command line, and run laser processing thread if applicable
  // for that laser class.  For the purposes of this demo, add all
  // possible lasers to ArRobot's list rather than just the ones that were
  // connected by this call so when you enter laser mode, you
  // can then interactively choose which laser to use from that list of all
  // lasers mentioned in robot parameters and on command line. Normally,
  // only connected lasers are put in ArRobot's list.
  if (!laserConnector.connectLasers(
        false,  // continue after connection failures
        false,  // add only connected lasers to ArRobot
        true    // add all lasers to ArRobot
  ))
  {
     printf("Warning: Could not connect to laser(s). Set LaserAutoConnect to false in this robot's individual parameter file to disable laser connection.\n");
  }

/* not needed, robot connector will do it by default
  if (!sonarConnector.connectSonars(
        false,  // continue after connection failures
        false,  // add only connected lasers to ArRobot
        true    // add all lasers to ArRobot
  ))
  {
    printf("Could not connect to sonars... exiting\n");
    Aria::exit(2);
  }
*/

  // Create and connect to the compass if the robot has one.
//  ArTCM2 *compass = compassConnector.create(&robot);
//  if(compass && !compass->blockingConnect()) {
//    compass = NULL;
//  }
  
  // Sleep for a second so some messages from the initial responses
  // from robots and cameras and such can catch up
  ArUtil::sleep(1000);

  // We need to lock the robot since we'll be setting up these modes
  // while the robot task loop thread is already running, and they 
  // need to access some shared data in ArRobot.
  robot.lock();

  // now add all the modes for this demo
  // these classes are defined in ArModes.cpp in ARIA's source code.
  
  if(robot.getOrigRobotConfig()->getHasGripper())
    new ArModeGripper(&robot, "gripper", 'g', 'G');
  else
    ArLog::log(ArLog::Normal, "Robot does not indicate that it has a gripper.");
  //ArModeActs actsMode(&robot, "acts", 'a', 'A');
  //ArModeTCM2 tcm2(&robot, "tcm2", 'm', 'M', compass);
  ArModeIO io(&robot, "io", 'i', 'I');
  ArModeRobotStatus stat(&robot, "detailed status/error flags", 'f', 'F');
  ArModeConfig cfg(&robot, "report robot config", 'o' , 'O');
  ArModeCommand command(&robot, "command", 'd', 'D');
  ArModeCamera camera(&robot, "camera", 'c', 'C');
  ArModePosition position(&robot, "position", 'p', 'P', &gyro);
  ArModeSonar sonar(&robot, "sonar", 's', 'S');
  ArModeBumps bumps(&robot, "bumps", 'b', 'B');
  ArModeLaser laser(&robot, "laser", 'l', 'L');
  ArModeWander wander(&robot, "wander", 'w', 'W');
  ArModeUnguardedTeleop unguardedTeleop(&robot, "unguarded teleop", 'u', 'U');
  ArModeTeleop teleop(&robot, "teleop", 't', 'T');


  // activate the default mode
  teleop.activate();

  // turn on the motors
  robot.comInt(ArCommands::ENABLE, 1);

  robot.unlock();
  
  // Block execution of the main thread here and wait for the robot's task loop
  // thread to exit (e.g. by robot disconnecting, escape key pressed, or OS
  // signal)
  robot.waitForRunExit();

  Aria::exit(0);
  return 0;

}



ArMode *ArMode::ourActiveMode = NULL;
ArGlobalFunctor *ArMode::ourHelpCB = NULL;
std::list<ArMode *> ArMode::ourModes;

/**
   @param robot the robot we're attaching to
   
   @param name the name of this mode

   @param key the primary key to switch to this mode on... it can be
   '\\0' if you don't want to use this

   @param key2 an alternative key to switch to this mode on... it can be
   '\\0' if you don't want a second alternative key
**/
ArMode::ArMode(ArRobot *robot, const char *name, char key, 
			char key2) :
  myActivateCB(this, &ArMode::activate),
  myDeactivateCB(this, &ArMode::deactivate),
  myUserTaskCB(this, &ArMode::userTask)
{
  ArKeyHandler *keyHandler;
  myName = name;
  myRobot = robot;
  myKey = key;
  myKey2 = key2;

  myActivateCB.setName(myName + " mode activate callback");
  myDeactivateCB.setName(myName + " mode deactivate callback");
  myUserTaskCB.setName(myName + " mode user task");

  // see if there is already a keyhandler, if not make one for ourselves
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    ArLog::log(ArLog::Normal, "ArMode::ArMode: Warning: no key handler yet in program, creating a second one.");
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    if (myRobot != NULL)
      myRobot->attachKeyHandler(keyHandler);
    else
      ArLog::log(ArLog::Terse, "ArMode: No robot to attach a keyHandler to, keyHandling won't work... either make your own keyHandler and drive it yourself, make a keyhandler and attach it to a robot, or give this a robot to attach to.");
  }  
  if (ourHelpCB == NULL)
  {
    ourHelpCB = new ArGlobalFunctor(&ArMode::baseHelp);
    if (!keyHandler->addKeyHandler('h', ourHelpCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for 'h', ArMode will not be invoked on an 'h' keypress.");
    if (!keyHandler->addKeyHandler('H', ourHelpCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for 'H', ArMode will not be invoked on an 'H' keypress.");
    if (!keyHandler->addKeyHandler('?', ourHelpCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for '?', ArMode will not be invoked on an '?' keypress.");
    if (!keyHandler->addKeyHandler('/', ourHelpCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for '/', ArMode will not be invoked on an '/' keypress.");

  }

  // now that we have one, add our keys as callbacks, print out big
  // warning messages if they fail
  if (myKey != '\0')
    if (!keyHandler->addKeyHandler(myKey, &myActivateCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for '%c', ArMode will not work correctly.", myKey);
  if (myKey2 != '\0')
    if (!keyHandler->addKeyHandler(myKey2, &myActivateCB))
      ArLog::log(ArLog::Terse, "The key handler already has a key for '%c', ArMode will not work correctly.", myKey2);

  // toss this mode into our list of modes
  ourModes.push_front(this);
}

ArMode::~ArMode()
{
  ArKeyHandler *keyHandler;
  if ((keyHandler = Aria::getKeyHandler()) != NULL)
  {
    if (myKey != '\0')
      keyHandler->remKeyHandler(myKey);
    if (myKey2 != '\0')
      keyHandler->remKeyHandler(myKey2);
  }
  if (myRobot != NULL)
    myRobot->remUserTask(&myUserTaskCB);
}

/** 
   Inheriting modes must first call this to get their user task called
   and to deactivate the active mode.... if it returns false then the
   inheriting class must return, as it means that his mode is already
   active
**/
bool ArMode::baseActivate()
{
  if (ourActiveMode == this)
    return false;
  myRobot->deactivateActions();
  if (myRobot != NULL)
  {
    myRobot->addUserTask((myName+"_mode_usertask").c_str(), 50, &myUserTaskCB);
  }
  if (ourActiveMode != NULL)
    ourActiveMode->deactivate();
  ourActiveMode = this;
  if (myRobot != NULL)
  {
    myRobot->stop();
    myRobot->clearDirectMotion();
  }
  
  baseHelp();
  return true;
}

/**
   This gets called when the mode is deactivated, it removes the user
   task from the robot
**/
bool ArMode::baseDeactivate()
{
  if (myRobot != NULL)
    myRobot->remUserTask(&myUserTaskCB);
  if (ourActiveMode == this)
  {
    ourActiveMode = NULL;
    return true;
  }
  return false;
}

const char *ArMode::getName()
{
  return myName.c_str();
}

char ArMode::getKey()
{
  return myKey;
}

char ArMode::getKey2()
{
  return myKey2;
}

void ArMode::baseHelp()
{
  std::list<ArMode *>::iterator it;
  ArLog::log(ArLog::Terse, "\n\nYou can do these actions with these keys:\n");
  ArLog::log(ArLog::Terse, "quit: escape");
  ArLog::log(ArLog::Terse, "help: 'h' or 'H' or '?' or '/'");
  ArLog::log(ArLog::Terse, "\nYou can switch to other modes with these keys:");
  for (it = ourModes.begin(); it != ourModes.end(); ++it)
  {
    ArLog::log(ArLog::Terse, "%30s mode: '%c' or '%c'", (*it)->getName(), 
	       (*it)->getKey(), (*it)->getKey2());
  }
  if (ourActiveMode == NULL)
    ArLog::log(ArLog::Terse, "You are in no mode currently.");
  else
  {
    ArLog::log(ArLog::Terse, "You are in '%s' mode currently.\n",
	       ourActiveMode->getName());
    ourActiveMode->help();
  }
}

void ArMode::addKeyHandler(int keyToHandle, ArFunctor *functor)
{
  ArKeyHandler *keyHandler;
  std::string charStr;

  // see if there is already a keyhandler, if not something is wrong
  // (since constructor should make one if there isn't one yet
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    ArLog::log(ArLog::Terse,"ArMode '%s'::keyHandler: There should already be a key handler, but there isn't... mode won't work right.", getName());
    return;
  }

  if (!keyHandler->addKeyHandler(keyToHandle, functor))
  {
    bool specialKey = true;
    switch (keyToHandle) {
    case ArKeyHandler::UP:
      charStr = "Up";
      break;
    case ArKeyHandler::DOWN:
      charStr = "Down";
      break;
    case ArKeyHandler::LEFT:
      charStr = "Left";
      break;
    case ArKeyHandler::RIGHT:
      charStr = "Right";
      break;
    case ArKeyHandler::ESCAPE:
      charStr = "Escape";
      break;
    case ArKeyHandler::F1:
      charStr = "F1";
      break;
    case ArKeyHandler::F2:
      charStr = "F2";
      break;
    case ArKeyHandler::F3:
      charStr = "F3";
      break;
    case ArKeyHandler::F4:
      charStr = "F4";
      break;
    case ArKeyHandler::SPACE:
      charStr = "Space";
      break;
    case ArKeyHandler::TAB:
      charStr = "Tab";
      break;
    case ArKeyHandler::ENTER:
      charStr = "Enter";
      break;
    case ArKeyHandler::BACKSPACE:
      charStr = "Backspace";
      break;
    default:
      charStr = (char)keyToHandle;
      specialKey = false;
      break;
    }
    if (specialKey || (keyToHandle >= '!' && keyToHandle <= '~'))
      ArLog::log(ArLog::Terse,  
		 "ArMode '%s': The key handler has a duplicate key for '%s' so the mode may not work right.", getName(), charStr.c_str());
    else
      ArLog::log(ArLog::Terse,  
		 "ArMode '%s': The key handler has a duplicate key for number %d so the mode may not work right.", getName(), keyToHandle);
  }
  
}

void ArMode::remKeyHandler(ArFunctor *functor)
{
  ArKeyHandler *keyHandler;
  std::string charStr;

  // see if there is already a keyhandler, if not something is wrong
  // (since constructor should make one if there isn't one yet
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    ArLog::log(ArLog::Terse,"ArMode '%s'::keyHandler: There should already be a key handler, but there isn't... mode won't work right.", getName());
    return;
  }
  if (!keyHandler->remKeyHandler(functor))
    ArLog::log(ArLog::Terse,  
	       "ArMode '%s': The key handler already didn't have the given functor so the mode may not be working right.", getName());
}
  

/// return unsigned byte as string of 8 '1' and '0' characters (MSB first, so
/// bit 0 will be last character in string, bit 7 will be first character.)
/// @todo generalize with others to any number of bits
std::string byte_as_bitstring(unsigned char byte) 
{
  char tmp[9];
  int bit; 
  int ch;
  for(bit = 7, ch = 0; bit >= 0; bit--,ch++)
    tmp[ch] = ((byte>>bit)&1) ? '1' : '0';
  tmp[8] = 0;
  return std::string(tmp);
}

/// return unsigned 16-bit value as string of 16 '1' and '0' characters (MSB first, so
/// bit 0 will be last character in string, bit 15 will be first character.)
/// @todo separate every 8 bits. 
/// @todo generalize with others to any number of bits
std::string int16_as_bitstring(int16_t n) 
{
  char tmp[17];
  int bit;
  int ch;
  for(bit = 15, ch = 0; bit >= 0; bit--, ch++)
    tmp[ch] = ((n>>bit)&1) ? '1' : '0';
  tmp[16] = 0;
  return std::string(tmp);
}

/// @todo separate every 8 bits. 
/// @todo generalize with others to any number of bits
std::string int32_as_bitstring(int32_t n)
{
  char tmp[33];
  int bit;
  int ch;
  for(bit = 31, ch = 0; bit >= 0; bit--, ch++)
    tmp[ch] = ((n>>bit)&1) ? '1' : '0';
  tmp[32] = 0;
  return std::string(tmp);
}

/** 
  @param robot ArRobot instance to be associate with
  @param name name of this mode
  @param key keyboard key that activates this mode
  @param key2 another keyboard key that activates this mode
*/
ArModeTeleop::ArModeTeleop(ArRobot *robot, const char *name, char key, char key2): 
  ArMode(robot, name, key, key2),
  myGroup(robot),
  myEnableMotorsCB(robot, &ArRobot::enableMotors)
{
  myEnableMotorsCB.setName("teleop mode enable motors key callback -> robot.enableMotors()");
  myGroup.deactivate();
}

ArModeTeleop::~ArModeTeleop()
{
  
}

void ArModeTeleop::activate()
{
  if (!baseActivate())
    return;
  addKeyHandler('e', &myEnableMotorsCB);
  myGroup.activateExclusive();
}

void ArModeTeleop::deactivate()
{
  remKeyHandler(&myEnableMotorsCB);
  if (!baseDeactivate())
    return;
  myGroup.deactivate();
}

void ArModeTeleop::help()
{
  ArLog::log(ArLog::Terse, 
	   "Teleop mode will drive under your joystick or keyboard control.");
  ArLog::log(ArLog::Terse, 
	     "It will not allow you to drive into obstacles it can see,");
  ArLog::log(ArLog::Terse, 
      "though if you are presistent you may be able to run into something.");
  ArLog::log(ArLog::Terse, "For joystick, hold in the trigger button and then move the joystick to drive.");
  ArLog::log(ArLog::Terse, "For keyboard control these are the keys and their actions:");
  ArLog::log(ArLog::Terse, "%13s:  speed up if forward or no motion, slow down if going backwards", "up arrow");
  ArLog::log(ArLog::Terse, "%13s:  slow down if going forwards, speed up if backward or no motion", "down arrow");
  ArLog::log(ArLog::Terse, "%13s:  turn left", "left arrow");
  ArLog::log(ArLog::Terse, "%13s:  turn right", "right arrow");
  if (myRobot->hasLatVel())
  {
    ArLog::log(ArLog::Terse, "%13s:  move left", "z");
    ArLog::log(ArLog::Terse, "%13s:  move right", "x");
  }
  ArLog::log(ArLog::Terse, "%13s:  stop", "space bar");
  ArLog::log(ArLog::Terse, "%13s:  (re)enable motors", "e");
  if (!myRobot->hasLatVel())
    printf("%10s %10s %10s %10s %10s %10s", "transVel", "rotVel", "x", "y", "th", "volts");
  else
    printf("%10s %10s %10s %10s %10s %10s %10s", "transVel", "rotVel", "latVel", "x", "y", "th", "volts");
  if(myRobot->haveStateOfCharge())
    printf(" %10s", "soc");
  printf(" %10s", ""); //flags
  printf("\n");
  fflush(stdout);
}

void ArModeTeleop::userTask()
{
  if (!myRobot->hasLatVel())
    printf("\r%10.0f %10.0f %10.0f %10.0f %10.1f %10.1f", myRobot->getVel(), 
	   myRobot->getRotVel(), myRobot->getX(), myRobot->getY(), 
	   myRobot->getTh(), myRobot->getRealBatteryVoltage());
  else
    printf("\r%9.0f %9.0f %9.0f %9.0f %9.0f %9.1f %9.1f", 
	   myRobot->getVel(), myRobot->getRotVel(), myRobot->getLatVel(),
	   myRobot->getX(), myRobot->getY(), myRobot->getTh(), 
	   myRobot->getRealBatteryVoltage());
  if(myRobot->haveStateOfCharge())
    printf(" %9.1f", myRobot->getStateOfCharge());
  if(myRobot->isEStopPressed()) printf(" [ESTOP]"); 
  if(myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled()) printf(" [STALL] "); 
  if(!myRobot->areMotorsEnabled()) printf(" [DISABLED] "); 
  
  // spaces to cover previous output
  if(!myRobot->isEStopPressed() || !(myRobot->isLeftMotorStalled() && myRobot->isRightMotorStalled()) || myRobot->areMotorsEnabled())
    printf("                 ");

  fflush(stdout);
}

ArModeUnguardedTeleop::ArModeUnguardedTeleop(ArRobot *robot,
						      const char *name, 
						      char key, char key2): 
  ArMode(robot, name, key, key2),
  myGroup(robot),
  myEnableMotorsCB(robot, &ArRobot::enableMotors)
{
  myGroup.deactivate();
}

ArModeUnguardedTeleop::~ArModeUnguardedTeleop()
{
  
}

void ArModeUnguardedTeleop::activate()
{
  if (!baseActivate())
    return;
  addKeyHandler('e', &myEnableMotorsCB);
  myGroup.activateExclusive();
}

void ArModeUnguardedTeleop::deactivate()
{
  remKeyHandler(&myEnableMotorsCB);
  if (!baseDeactivate())
    return;
  myGroup.deactivate();
}

void ArModeUnguardedTeleop::help()
{
  ArLog::log(ArLog::Terse, 
	   "Unguarded teleop mode will drive under your joystick or keyboard control.");
  ArLog::log(ArLog::Terse, 
	     "\n### THIS MODE IS UNGUARDED AND UNSAFE, BE CAREFUL DRIVING");
  ArLog::log(ArLog::Terse,
	     "\nAs it will allow you to drive into things or down stairs.");
  ArLog::log(ArLog::Terse, "For joystick, hold in the trigger button and then move the joystick to drive.");
  ArLog::log(ArLog::Terse, "For keyboard control these are the keys and their actions:");
  ArLog::log(ArLog::Terse, "%13s:  speed up if forward or no motion, slow down if going backwards", "up arrow");
  ArLog::log(ArLog::Terse, "%13s:  slow down if going forwards, speed up if backward or no motion", "down arrow");
  ArLog::log(ArLog::Terse, "%13s:  turn left", "left arrow");
  ArLog::log(ArLog::Terse, "%13s:  turn right", "right arrow");
  if (myRobot->hasLatVel())
  {
    ArLog::log(ArLog::Terse, "%13s:  move left", "z");
    ArLog::log(ArLog::Terse, "%13s:  move right", "x");
  }
  ArLog::log(ArLog::Terse, "%13s:  stop", "space bar");
  ArLog::log(ArLog::Terse, "%13s:  (re)enable motors", "e");
  if (!myRobot->hasLatVel())
    printf("%10s %10s %10s %10s %10s %10s", "transVel", "rotVel", "x", "y", "th", "volts");
  else
    printf("%10s %10s %10s %10s %10s %10s %10s", "transVel", "rotVel", "latVel", "x", "y", "th", "volts");
  if(myRobot->haveStateOfCharge())
    printf(" %10s", "soc");
  printf(" %10s", ""); //flags
  printf("\n");
  fflush(stdout);
}

void ArModeUnguardedTeleop::userTask()
{
  if (!myRobot->hasLatVel())
    printf("\r%9.0f %9.0f %9.0f %9.0f %9.1f %9.1f", myRobot->getVel(), 
	   myRobot->getRotVel(), myRobot->getX(), myRobot->getY(), 
	   myRobot->getTh(), myRobot->getRealBatteryVoltage());
  else
    printf("\r%9.0f %9.0f %9.0f %9.0f %9.0f %9.1f %9.1f", 
	   myRobot->getVel(), myRobot->getRotVel(), myRobot->getLatVel(),
	   myRobot->getX(), myRobot->getY(), myRobot->getTh(), 
	   myRobot->getRealBatteryVoltage());
  if(myRobot->haveStateOfCharge())
    printf(" %9.1f", myRobot->getStateOfCharge());
  if(myRobot->isEStopPressed()) printf(" [ESTOP] ");
  if(myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled()) printf(" [STALL] ");
  if(!myRobot->areMotorsEnabled()) printf(" [DISABLED] ");

  // spaces to cover previous output
  if(!myRobot->isEStopPressed() || !(myRobot->isLeftMotorStalled() && myRobot->isRightMotorStalled()) || myRobot->areMotorsEnabled())
    printf("                 ");

  fflush(stdout);
}

ArModeWander::ArModeWander(ArRobot *robot, const char *name, char key, char key2): 
  ArMode(robot, name, key, key2),
  myGroup(robot)
{
  myGroup.deactivate();
}

ArModeWander::~ArModeWander()
{
  
}

void ArModeWander::activate()
{
  if (!baseActivate())
    return;
  myGroup.activateExclusive();
}

void ArModeWander::deactivate()
{
  if (!baseDeactivate())
    return;
  myGroup.deactivate(); 
}

void ArModeWander::help()
{
  ArLog::log(ArLog::Terse, "Wander mode will simply drive around forwards until it finds an obstacle,");
  ArLog::log(ArLog::Terse, "then it will turn until its clear, and continue.");
  printf("%10s %10s %10s %10s %10s %10s\n", "transVel", "rotVel", "x", "y", "th", "volts");
  fflush(stdout);
}

void ArModeWander::userTask()
{
  printf("\r%10.0f %10.0f %10.0f %10.0f %10.1f %10.1f", myRobot->getVel(), 
	 myRobot->getRotVel(), myRobot->getX(), myRobot->getY(), 
	 myRobot->getTh(), myRobot->getRealBatteryVoltage());
  fflush(stdout);
}

ArModeGripper::ArModeGripper(ArRobot *robot, const char *name, 
				      char key, char key2): 
  ArMode(robot, name, key, key2),
  myGripper(robot),
  myOpenCB(this, &ArModeGripper::open),
  myCloseCB(this, &ArModeGripper::close),
  myUpCB(this, &ArModeGripper::up),
  myDownCB(this, &ArModeGripper::down),
  myStopCB(this, &ArModeGripper::stop),
  myExerciseCB(this, &ArModeGripper::exercise)
{
  myExercising = false;
}

ArModeGripper::~ArModeGripper()
{
  
}

void ArModeGripper::activate()
{
  if (!baseActivate())
    return;

  addKeyHandler(ArKeyHandler::UP, &myUpCB);
  addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  addKeyHandler(ArKeyHandler::RIGHT, &myOpenCB);
  addKeyHandler(ArKeyHandler::LEFT, &myCloseCB);  
  addKeyHandler(ArKeyHandler::SPACE, &myStopCB);
  addKeyHandler('e', &myExerciseCB);
  addKeyHandler('E', &myExerciseCB);
}

void ArModeGripper::deactivate()
{
  if (!baseDeactivate())
    return;

  remKeyHandler(&myUpCB);
  remKeyHandler(&myDownCB);
  remKeyHandler(&myOpenCB);
  remKeyHandler(&myCloseCB);
  remKeyHandler(&myStopCB);
  remKeyHandler(&myExerciseCB);
}

void ArModeGripper::userTask()
{
  int val;
  printf("\r");
  if (myGripper.getBreakBeamState() & 2) // outer 
    printf("%13s", "blocked");
  else 
    printf("%13s", "clear");
  if (myGripper.getBreakBeamState() & 1) // inner
    printf("%13s", "blocked");
  else 
    printf("%13s", "clear");
  val = myGripper.getGripState(); // gripper portion
  if (val == 0)
    printf("%13s", "between");
  else if (val == 1)
    printf("%13s", "open");
  else if (val == 2)
    printf("%13s", "closed");
  if (myGripper.isLiftMaxed()) // lift
    printf("%13s", "maxed");
  else
    printf("%13s", "clear");
  val = myGripper.getPaddleState(); // paddle section
  if (val & 1) // left paddle
    printf("%13s", "triggered");
  else
    printf("%13s", "clear");
  if (val & 2) // right paddle
    printf("%13s", "triggered");
  else
    printf("%13s", "clear");
  fflush(stdout);

  // exercise the thing
  if (myExercising)
  {
    switch (myExerState) {
    case UP_OPEN:
      if ((myLastExer.mSecSince() > 3000 && myGripper.isLiftMaxed()) ||
	  myLastExer.mSecSince() > 30000)
      {
	myGripper.gripClose();
	myExerState = UP_CLOSE;
	myLastExer.setToNow();
	if (myLastExer.mSecSince() > 30000)
	  ArLog::log(ArLog::Terse, "\nLift took more than thirty seconds to raise, there is probably a problem with it.\n");
      }
      break;
    case UP_CLOSE:
      if (myGripper.getGripState() == 2 || myLastExer.mSecSince() > 10000)
      {
	myGripper.liftDown();
	myExerState = DOWN_CLOSE;
	myLastExer.setToNow();
	if (myLastExer.mSecSince() > 10000)
	  ArLog::log(ArLog::Terse, "\nGripper took more than 10 seconds to close, there is probably a problem with it.\n");
      }
      break;
    case DOWN_CLOSE:
      if ((myLastExer.mSecSince() > 3000 && myGripper.isLiftMaxed()) ||
	  myLastExer.mSecSince() > 30000)
      {
	myGripper.gripOpen();
	myExerState = DOWN_OPEN;
	myLastExer.setToNow();
	if (myLastExer.mSecSince() > 30000)
	  ArLog::log(ArLog::Terse, "\nLift took more than thirty seconds to raise, there is probably a problem with it.\n");
      }
      break;
    case DOWN_OPEN:
      if (myGripper.getGripState() == 1 || myLastExer.mSecSince() > 10000)
      {
	myGripper.liftUp();
	myExerState = UP_OPEN;
	myLastExer.setToNow();
	if (myLastExer.mSecSince() > 10000)
	  ArLog::log(ArLog::Terse, "\nGripper took more than 10 seconds to open, there is probably a problem with it.\n");
      }
      break;
    }      
    
  }
}

void ArModeGripper::open()
{
  if (myExercising == true)
  {
    myExercising = false;
    myGripper.gripperHalt();
  }
  myGripper.gripOpen();
}

void ArModeGripper::close()
{
  if (myExercising == true)
  {
    myExercising = false;
    myGripper.gripperHalt();
  }
  myGripper.gripClose();
}

void ArModeGripper::up()
{
  if (myExercising == true)
  {
    myExercising = false;
    myGripper.gripperHalt();
  }
  myGripper.liftUp();
}

void ArModeGripper::down()
{
  if (myExercising == true)
  {
    myExercising = false;
    myGripper.gripperHalt();
  }
  myGripper.liftDown();
}

void ArModeGripper::stop()
{
  if (myExercising == true)
  {
    myExercising = false;
    myGripper.gripperHalt();
  }
  myGripper.gripperHalt();
}

void ArModeGripper::exercise()
{
  if (myExercising == false)
  {
    ArLog::log(ArLog::Terse, 
       "\nGripper will now be exercised until another command is given.");
    myExercising = true;
    myExerState = UP_OPEN;
    myGripper.liftUp();
    myGripper.gripOpen();
    myLastExer.setToNow();
  }
}

void ArModeGripper::help()
{
  ArLog::log(ArLog::Terse, 
	     "Gripper mode will let you control or exercise the gripper.");
  ArLog::log(ArLog::Terse, 
      "If you start exercising the gripper it will stop your other commands.");
  ArLog::log(ArLog::Terse, 
	     "If you use other commands it will interrupt the exercising.");
  ArLog::log(ArLog::Terse, "%13s:  raise lift", "up arrow");
  ArLog::log(ArLog::Terse, "%13s:  lower lift", "down arrow");
  ArLog::log(ArLog::Terse, "%13s:  close gripper paddles", "left arrow");
  ArLog::log(ArLog::Terse, "%13s:  open gripper paddles", "right arrow");
  ArLog::log(ArLog::Terse, "%13s:  stop gripper paddles and lift", 
	     "space bar");
  ArLog::log(ArLog::Terse, "%13s:  exercise the gripper", "'e' or 'E'");
  ArLog::log(ArLog::Terse, "\nGripper status:");
  ArLog::log(ArLog::Terse, "%13s%13s%13s%13s%13s%13s", "BB outer", "BB inner",
	     "Paddles", "Lift", "LeftPaddle", "RightPaddle");
  
}



ArModeCamera::ArModeCamera(ArRobot *robot, const char *name, 
				      char key, char key2): 
  ArMode(robot, name, key, key2),
  myUpCB(this, &ArModeCamera::up),
  myDownCB(this, &ArModeCamera::down),
  myLeftCB(this, &ArModeCamera::left),
  myRightCB(this, &ArModeCamera::right),
  myCenterCB(this, &ArModeCamera::center),
  myZoomInCB(this, &ArModeCamera::zoomIn),  
  myZoomOutCB(this, &ArModeCamera::zoomOut),
  myExerciseCB(this, &ArModeCamera::exercise),
//  mySonyCB(this, &ArModeCamera::sony),
  myCanonCB(this, &ArModeCamera::canon),
  myDpptuCB(this, &ArModeCamera::dpptu),
//  myAmptuCB(this, &ArModeCamera::amptu),
  myCanonInvertedCB(this, &ArModeCamera::canonInverted),
//  mySonySerialCB(this, &ArModeCamera::sonySerial),
  myCanonSerialCB(this, &ArModeCamera::canonSerial),
  myDpptuSerialCB(this, &ArModeCamera::dpptuSerial),
//  myAmptuSerialCB(this, &ArModeCamera::amptuSerial),
  myCanonInvertedSerialCB(this, &ArModeCamera::canonInvertedSerial),
  myRVisionSerialCB(this, &ArModeCamera::rvisionSerial),
  myCom1CB(this, &ArModeCamera::com1),
  myCom2CB(this, &ArModeCamera::com2),
  myCom3CB(this, &ArModeCamera::com3),
  myCom4CB(this, &ArModeCamera::com4),
  myUSBCom0CB(this, &ArModeCamera::usb0),
  myUSBCom9CB(this, &ArModeCamera::usb9),
  myAux1CB(this, &ArModeCamera::aux1),
  myAux2CB(this, &ArModeCamera::aux2),
  myPanAmount(5),
  myTiltAmount(3),
  myAutoFocusOn(true),
  myToggleAutoFocusCB(this, &ArModeCamera::toggleAutoFocus)
{
  myState = STATE_CAMERA;
  myExercising = false;
}

ArModeCamera::~ArModeCamera()
{
  
}

void ArModeCamera::activate()
{
  ArKeyHandler *keyHandler;
  if (!baseActivate())
    return;
  // see if there is already a keyhandler, if not something is wrong
  // (since constructor should make one if there isn't one yet
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    ArLog::log(ArLog::Terse,"ArModeCamera::activate: There should already be a key handler, but there isn't... mode won't work");
    return;
  }
  if (myState == STATE_CAMERA)
    takeCameraKeys();
  else if (myState == STATE_PORT)
    takePortKeys();
  else if (myState == STATE_MOVEMENT)
    takeMovementKeys();
  else
    ArLog::log(ArLog::Terse,"ArModeCamera in bad state.");
  
}

void ArModeCamera::deactivate()
{
  if (!baseDeactivate())
    return;
  if (myState == STATE_CAMERA)
    giveUpCameraKeys();
  else if (myState == STATE_PORT)
    giveUpPortKeys();
  else if (myState == STATE_MOVEMENT)
    giveUpMovementKeys();
  else
    ArLog::log(ArLog::Terse,"ArModeCamera in bad state.");
}

void ArModeCamera::userTask()
{
  if (myExercising && myCam != NULL && myLastExer.mSecSince() > 7000)
  {
    switch (myExerState) {
    case CENTER:
      myCam->panTilt(myCam->getMaxNegPan(), myCam->getMaxPosTilt());
      myExerState = UP_LEFT;
      myLastExer.setToNow();
      break;
    case UP_LEFT:
      myCam->panTilt(myCam->getMaxPosPan(), myCam->getMaxPosTilt());
      myExerState = UP_RIGHT;
      myLastExer.setToNow();
      break;
    case UP_RIGHT:
      myCam->panTilt(myCam->getMaxPosPan(), myCam->getMaxNegTilt());
      myExerState = DOWN_RIGHT;
      myLastExer.setToNow();
      break;
    case DOWN_RIGHT:
      myCam->panTilt(myCam->getMaxNegPan(), myCam->getMaxNegTilt());
      myExerState = DOWN_LEFT;
      myLastExer.setToNow();
      break;
    case DOWN_LEFT:
      myCam->panTilt(0, 0);
      myExerState = CENTER;
      myLastExer.setToNow();
      break;
    }      
  }
  if (myExercising && myCam != NULL && myCam->canZoom() && 
      myLastExerZoomed.mSecSince() > 35000)
  {
    if (myExerZoomedIn)
      myCam->zoom(myCam->getMinZoom());
    else
      myCam->zoom(myCam->getMaxZoom());
    myLastExerZoomed.setToNow();
  }
}

void ArModeCamera::left()
{
  if (myExercising == true)
    myExercising = false;
  myCam->panRel(-myPanAmount);
}

void ArModeCamera::right()
{
  if (myExercising == true)
    myExercising = false;
  myCam->panRel(myPanAmount);
}

void ArModeCamera::up()
{
  if (myExercising == true)
    myExercising = false;
  myCam->tiltRel(myTiltAmount);
}

void ArModeCamera::down()
{  
  if (myExercising == true)
    myExercising = false;
  myCam->tiltRel(-myTiltAmount);
}

void ArModeCamera::center()
{
  if (myExercising == true)
    myExercising = false;
  myCam->panTilt(0, 0);
  myCam->zoom(myCam->getMinZoom());
}

void ArModeCamera::exercise()
{
  if (myExercising == false)
  {
    ArLog::log(ArLog::Terse, 
       "Camera will now be exercised until another command is given.");
    myExercising = true;
    myExerState = UP_LEFT;
    myLastExer.setToNow();
    myCam->panTilt(myCam->getMaxNegPan(), myCam->getMaxPosTilt());
    myLastExerZoomed.setToNow();
    myExerZoomedIn = true;
    if (myCam->canZoom())
      myCam->zoom(myCam->getMaxZoom());
  }
}

void ArModeCamera::toggleAutoFocus()
{
  ArLog::log(ArLog::Terse, "Turning autofocus %s", myAutoFocusOn?"off":"on");
  if(myCam->setAutoFocus(!myAutoFocusOn))
    myAutoFocusOn = !myAutoFocusOn;
}

void ArModeCamera::help()
{
  ArLog::log(ArLog::Terse, 
	     "Camera mode will let you control or exercise the camera.");
  ArLog::log(ArLog::Terse, 
      "If you start exercising the camera it will stop your other commands.");
  if (myState == STATE_CAMERA)
    helpCameraKeys();
  else if (myState == STATE_PORT)
    helpPortKeys();
  else if (myState == STATE_MOVEMENT)
    helpMovementKeys();
  else
    ArLog::log(ArLog::Terse, "Something is horribly wrong and mode camera is in no state.");
}

void ArModeCamera::zoomIn()
{
  if (myCam->canZoom())
  {
    myCam->zoom(myCam->getZoom() + 
	 ArMath::roundInt((myCam->getMaxZoom() - myCam->getMinZoom()) * .01));
  }
}

void ArModeCamera::zoomOut()
{
  if (myCam->canZoom())
  {
    myCam->zoom(myCam->getZoom() - 
	ArMath::roundInt((myCam->getMaxZoom() - myCam->getMinZoom()) * .01));
  }
}

/*
void ArModeCamera::sony()
{
  myCam = new ArSonyPTZ(myRobot);
  ArLog::log(ArLog::Terse, "\nSony selected, now need to select the aux port.");
  cameraToAux();
}
*/

void ArModeCamera::canon()
{
  myCam = new ArVCC4(myRobot);
  ArLog::log(ArLog::Terse, "\nCanon selected, now need to select the aux port.");
  cameraToAux();
}

void ArModeCamera::dpptu()
{
  myCam = new ArDPPTU(myRobot);
  ArLog::log(ArLog::Terse, "\nDPPTU selected, now need to select the aux port.");
  cameraToAux();
}

/*
void ArModeCamera::amptu()
{
  myCam = new ArAMPTU(myRobot);
  ArLog::log(ArLog::Terse, 
	     "\nActivMedia Pan Tilt Unit selected, now need to select the aux port.");
  cameraToAux();
}
*/

void ArModeCamera::canonInverted()
{
  myCam = new ArVCC4(myRobot, true);
  ArLog::log(ArLog::Terse, "\nInverted Canon selected, now need to select the aux port.");
  cameraToAux();
}

/*
void ArModeCamera::sonySerial()
{
  myCam = new ArSonyPTZ(myRobot);
  ArLog::log(ArLog::Terse, "\nSony selected, now need to select serial port.");
  cameraToPort();
}
*/

void ArModeCamera::canonSerial()
{
  myCam = new ArVCC4(myRobot);
  ArLog::log(ArLog::Terse, 
	     "\nCanon VCC4 selected, now need to select serial port.");
  cameraToPort();
}

void ArModeCamera::dpptuSerial()
{
  myCam = new ArDPPTU(myRobot);
  ArLog::log(ArLog::Terse, "\nDPPTU selected, now need to select serial port.");
  cameraToPort();
}

/*
void ArModeCamera::amptuSerial()
{
  myCam = new ArAMPTU(myRobot);
  ArLog::log(ArLog::Terse, "\nAMPTU selected, now need to select serial port.");
  cameraToPort();
}
*/

void ArModeCamera::canonInvertedSerial()
{
  myCam = new ArVCC4(myRobot, true);
  ArLog::log(ArLog::Terse, 
	     "\nInverted Canon VCC4 selected, now need to select serial port.");
  cameraToPort();
}

void ArModeCamera::rvisionSerial()
{
  myCam = new ArRVisionPTZ(myRobot);
  ArLog::log(ArLog::Terse, "\nRVision selected, now need to select serial port.");
  cameraToPort();
}

void ArModeCamera::com1()
{
  myConn.setPort(ArUtil::COM1);
  portToMovement();
}

void ArModeCamera::com2()
{
  myConn.setPort(ArUtil::COM2);
  portToMovement();
}

void ArModeCamera::com3()
{
  myConn.setPort(ArUtil::COM3);
  portToMovement();
}

void ArModeCamera::com4()
{
  myConn.setPort(ArUtil::COM4);
  portToMovement();
}

void ArModeCamera::usb0()
{
  myConn.setPort("/dev/ttyUSB0");
  portToMovement();
}

void ArModeCamera::usb9()
{
  myConn.setPort("/dev/ttyUSB9");
  portToMovement();
}

void ArModeCamera::aux1()
{
  myCam->setAuxPort(1);
  auxToMovement();
}
void ArModeCamera::aux2()
{
  myCam->setAuxPort(2);
  auxToMovement();
}

void ArModeCamera::cameraToMovement()
{
  myState = STATE_MOVEMENT;
  myCam->init();
  myRobot->setPTZ(myCam);
  giveUpCameraKeys();
  takeMovementKeys();
  helpMovementKeys();
}

void ArModeCamera::cameraToPort()
{
  myState = STATE_PORT;
  giveUpCameraKeys();
  takePortKeys();
  helpPortKeys();
}

void ArModeCamera::cameraToAux()
{
  giveUpCameraKeys();
  takeAuxKeys();
  helpAuxKeys();
}

void ArModeCamera::portToMovement()
{
  ArLog::log(ArLog::Normal, "ArModeCamera: Opening connection to camera on port %s", myConn.getPortName());
  if (!myConn.openSimple())
  {
    ArLog::log(ArLog::Terse, 
	       "\n\nArModeCamera: Could not open camera on that port, try another port.\n");
    helpPortKeys();
    return;
  }
  if(!myCam->setDeviceConnection(&myConn))
  {
    ArLog::log(ArLog::Terse, "\n\nArModeCamera: Error setting device connection!\n");
    return;
  }
  myCam->init();
  myRobot->setPTZ(myCam);
  myState = STATE_MOVEMENT;
  giveUpPortKeys();
  takeMovementKeys();
  helpMovementKeys();
}

void ArModeCamera::auxToMovement()
{
  myCam->init();
  myRobot->setPTZ(myCam);
  myState = STATE_MOVEMENT;
  giveUpAuxKeys();
  takeMovementKeys();
  helpMovementKeys();
}

void ArModeCamera::takeCameraKeys()
{
//  addKeyHandler('1', &mySonyCB);
  addKeyHandler('1', &myCanonCB);
  addKeyHandler('2', &myDpptuCB);
//  addKeyHandler('4', &myAmptuCB);
  addKeyHandler('3', &myCanonInvertedCB);
//  addKeyHandler('!', &mySonySerialCB);
  addKeyHandler('4', &myCanonSerialCB);
  addKeyHandler('5', &myDpptuSerialCB);
//  addKeyHandler('$', &myAmptuSerialCB);
  addKeyHandler('6', &myCanonInvertedSerialCB);
  addKeyHandler('7', &myRVisionSerialCB);
}

void ArModeCamera::giveUpCameraKeys()
{
  remKeyHandler(&myCanonCB);
//  remKeyHandler(&mySonyCB);
  remKeyHandler(&myDpptuCB);
//  remKeyHandler(&myAmptuCB);
  remKeyHandler(&myCanonInvertedCB);
//  remKeyHandler(&mySonySerialCB);
  remKeyHandler(&myCanonSerialCB);
  remKeyHandler(&myDpptuSerialCB);
//  remKeyHandler(&myAmptuSerialCB);
  remKeyHandler(&myCanonInvertedSerialCB);
  remKeyHandler(&myRVisionSerialCB);
}

void ArModeCamera::helpCameraKeys()
{
  ArLog::log(ArLog::Terse, 
	     "You now need to select what type of camera you have.");
//  ArLog::log(ArLog::Terse, 
	//     "%13s: select a SONY PTZ camera attached to the robot", "'1'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select a Canon VCC4 camera attached to the robot", "'1'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select a DPPTU camera attached to the robot", "'2'");
//  ArLog::log(ArLog::Terse, 
//	     "%13s: select an AMPTU camera attached to the robot", "'4'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select an inverted Canon VCC4 camera attached to the robot", "'3'");

//  ArLog::log(ArLog::Terse, 
//	     "%13s: select a SONY PTZ camera attached to a serial port", 
//	     "'!'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select a Canon VCC4 camera attached to a serial port", 
	     "'4'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select a DPPTU camera attached to a serial port",
	     "'5'");
//  ArLog::log(ArLog::Terse, 
//	     "%13s: select an AMPTU camera attached to a serial port", 
//	     "'$'");
  ArLog::log(ArLog::Terse, 
	     "%13s: select an inverted Canon VCC4 camera attached to a serial port", 
	     "'6'");
  ArLog::log(ArLog::Terse,
	     "%13s: select an RVision camera attached to a serial port",
	     "'7'");
}

void ArModeCamera::takePortKeys()
{
  addKeyHandler('1', &myCom1CB);
  addKeyHandler('2', &myCom2CB);
  addKeyHandler('3', &myCom3CB);
  addKeyHandler('4', &myCom4CB);
  addKeyHandler('5', &myUSBCom0CB);
  addKeyHandler('6', &myUSBCom9CB);
}

void ArModeCamera::giveUpPortKeys()
{
  remKeyHandler(&myCom1CB);
  remKeyHandler(&myCom2CB);
  remKeyHandler(&myCom3CB);
  remKeyHandler(&myCom4CB);
  remKeyHandler(&myUSBCom0CB);
  remKeyHandler(&myUSBCom9CB);
}

void ArModeCamera::helpPortKeys()
{
  ArLog::log(ArLog::Terse, 
	     "You now need to select what port your camera is on.");
  ArLog::log(ArLog::Terse, "%13s:  select COM1 or /dev/ttyS0", "'1'");
  ArLog::log(ArLog::Terse, "%13s:  select COM2 or /dev/ttyS1", "'2'");
  ArLog::log(ArLog::Terse, "%13s:  select COM3 or /dev/ttyS2", "'3'");
  ArLog::log(ArLog::Terse, "%13s:  select COM4 or /dev/ttyS3", "'4'");
  ArLog::log(ArLog::Terse, "%13s:  select /dev/ttyUSB0", "'5'");
  ArLog::log(ArLog::Terse, "%13s:  select /dev/ttyUSB9", "'6'");
}

void ArModeCamera::takeAuxKeys()
{
  addKeyHandler('1', &myAux1CB);
  addKeyHandler('2', &myAux2CB);
}

void ArModeCamera::giveUpAuxKeys()
{
  remKeyHandler(&myAux1CB);
  remKeyHandler(&myAux2CB);
}

void ArModeCamera::helpAuxKeys()
{
  ArLog::log(ArLog::Terse,
             "You now need to select what aux port your camera is on.");
  ArLog::log(ArLog::Terse, "%13s:  select AUX1", "'1'");
  ArLog::log(ArLog::Terse, "%13s:  select AUX2", "'2'");
}

void ArModeCamera::takeMovementKeys()
{
  addKeyHandler(ArKeyHandler::UP, &myUpCB);
  addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  addKeyHandler(ArKeyHandler::LEFT, &myLeftCB);
  addKeyHandler(ArKeyHandler::RIGHT, &myRightCB);
  addKeyHandler(ArKeyHandler::SPACE, &myCenterCB);
  addKeyHandler('e', &myExerciseCB);
  addKeyHandler('E', &myExerciseCB);
  if (myCam->canZoom())
  {
    addKeyHandler('z', &myZoomInCB);
    addKeyHandler('Z', &myZoomInCB);
    addKeyHandler('x', &myZoomOutCB);
    addKeyHandler('X', &myZoomOutCB);
  }
  addKeyHandler('f', &myToggleAutoFocusCB);
  addKeyHandler('F', &myToggleAutoFocusCB);
}

void ArModeCamera::giveUpMovementKeys()
{
  remKeyHandler(&myUpCB);
  remKeyHandler(&myDownCB);
  remKeyHandler(&myLeftCB);
  remKeyHandler(&myRightCB);
  remKeyHandler(&myCenterCB);
  remKeyHandler(&myExerciseCB);
  if (myCam->canZoom())
  {
    remKeyHandler(&myZoomInCB);
    remKeyHandler(&myZoomOutCB);
  }
  remKeyHandler(&myToggleAutoFocusCB);
}

void ArModeCamera::helpMovementKeys()
{
  ArLog::log(ArLog::Terse, 
	     "Camera mode will now let you move the camera.");
  ArLog::log(ArLog::Terse, "%13s:  tilt camera up by %d", "up arrow", myTiltAmount);
  ArLog::log(ArLog::Terse, "%13s:  tilt camera down by %d", "down arrow", myTiltAmount);
  ArLog::log(ArLog::Terse, "%13s:  pan camera left by %d", "left arrow", myPanAmount);
  ArLog::log(ArLog::Terse, "%13s:  pan camera right by %d", "right arrow", myPanAmount);
  ArLog::log(ArLog::Terse, "%13s:  center camera and zoom out", 
	     "space bar");
  ArLog::log(ArLog::Terse, "%13s:  exercise the camera", "'e' or 'E'");
  if (myCam->canZoom())
  {
    ArLog::log(ArLog::Terse, "%13s:  zoom in", "'z' or 'Z'");
    ArLog::log(ArLog::Terse, "%13s:  zoom out", "'x' or 'X'");
  }
  ArLog::log(ArLog::Terse, "%13s:  toggle auto/fixed focus", "'f' or 'F'");
}

ArModeSonar::ArModeSonar(ArRobot *robot, const char *name, char key,
				  char key2) :
  ArMode(robot, name, key, key2),
  myAllSonarCB(this, &ArModeSonar::allSonar),
  myFirstSonarCB(this, &ArModeSonar::firstSonar),
  mySecondSonarCB(this, &ArModeSonar::secondSonar),
  myThirdSonarCB(this, &ArModeSonar::thirdSonar),
  myFourthSonarCB(this, &ArModeSonar::fourthSonar)
{
  myState = STATE_FIRST;
}

ArModeSonar::~ArModeSonar()
{

}

void ArModeSonar::activate()
{
  if (!baseActivate())
    return;
  addKeyHandler('1', &myAllSonarCB);
  addKeyHandler('2', &myFirstSonarCB);
  addKeyHandler('3', &mySecondSonarCB);
  addKeyHandler('4', &myThirdSonarCB);
  addKeyHandler('5', &myFourthSonarCB);
}

void ArModeSonar::deactivate()
{
  if (!baseDeactivate())
    return;
  remKeyHandler(&myAllSonarCB);
  remKeyHandler(&myFirstSonarCB);
  remKeyHandler(&mySecondSonarCB);
  remKeyHandler(&myThirdSonarCB);
  remKeyHandler(&myFourthSonarCB);
}

void ArModeSonar::help()
{
  int i;
  ArLog::log(ArLog::Terse, "This mode displays different segments of sonar.");
  ArLog::log(ArLog::Terse, 
	     "You can use these keys to switch what is displayed:");
  ArLog::log(ArLog::Terse, "%13s: display all sonar", "'1'");
  ArLog::log(ArLog::Terse, "%13s: display sonar 0 - 7", "'2'");
  ArLog::log(ArLog::Terse, "%13s: display sonar 8 - 15", "'3'");
  ArLog::log(ArLog::Terse, "%13s: display sonar 16 - 23", "'4'");
  ArLog::log(ArLog::Terse, "%13s: display sonar 24 - 31", "'5'");
  ArLog::log(ArLog::Terse, "Sonar readings:");
  if (myState == STATE_ALL)
  {
    ArLog::log(ArLog::Terse, "Displaying all sonar.");
    for (i = 0; i < myRobot->getNumSonar(); ++i)
      printf("%6d", i); 
  }
  else if (myState == STATE_FIRST)
  {
    ArLog::log(ArLog::Terse, "Displaying 0-7 sonar.");
    for (i = 0; i < myRobot->getNumSonar() && i <= 7; ++i)
      printf("%6d", i); 
  }
  else if (myState == STATE_SECOND)
  {
    ArLog::log(ArLog::Terse, "Displaying 8-15 sonar.");
    for (i = 8; i < myRobot->getNumSonar() && i <= 15; ++i)
      printf("%6d", i); 
  }
  else if (myState == STATE_THIRD)
  {
    ArLog::log(ArLog::Terse, "Displaying 16-23 sonar.");
    for (i = 16; i < myRobot->getNumSonar() && i <= 23; ++i)
      printf("%6d", i); 
  }
  else if (myState == STATE_FOURTH)
  {
    ArLog::log(ArLog::Terse, "Displaying 24-31 sonar.");
    for (i = 24; i < myRobot->getNumSonar() && i <= 31; ++i)
      printf("%6d", i); 
  }
  printf("\n");
}

void ArModeSonar::userTask()
{
  int i;
  printf("\r");
  if (myState == STATE_ALL)
  {
    for (i = 0; i < myRobot->getNumSonar(); ++i)
      printf("%6d", myRobot->getSonarRange(i)); 
  }
  else if (myState == STATE_FIRST)
  {
     for (i = 0; i < myRobot->getNumSonar() && i <= 7; ++i)
      printf("%6d", myRobot->getSonarRange(i));
  }
  else if (myState == STATE_SECOND)
  {
     for (i = 8; i < myRobot->getNumSonar() && i <= 15; ++i)
      printf("%6d", myRobot->getSonarRange(i));
  }
  else if (myState == STATE_THIRD)
  {
    for (i = 16; i < myRobot->getNumSonar() && i <= 23; ++i)
      printf("%6d", myRobot->getSonarRange(i)); 
  }
  else if (myState == STATE_FOURTH)
  {
    for (i = 24; i < myRobot->getNumSonar() && i <= 31; ++i)
      printf("%6d", myRobot->getSonarRange(i)); 
  }
  fflush(stdout);
}

void ArModeSonar::allSonar()
{
  myState = STATE_ALL;
  printf("\n");
  help();
}

void ArModeSonar::firstSonar()
{
  myState = STATE_FIRST;
  printf("\n");
  help();
}

void ArModeSonar::secondSonar()
{
  myState = STATE_SECOND;
  printf("\n");
  help();
}

void ArModeSonar::thirdSonar()
{
  myState = STATE_THIRD;
  printf("\n");
  help();
}

void ArModeSonar::fourthSonar()
{
  myState = STATE_FOURTH;
  printf("\n");
  help();
}

ArModeBumps::ArModeBumps(ArRobot *robot, const char *name, char key, char key2): 
  ArMode(robot, name, key, key2)
{
}

ArModeBumps::~ArModeBumps()
{
  
}

void ArModeBumps::activate()
{
  if (!baseActivate())
    return;
}

void ArModeBumps::deactivate()
{
  if (!baseDeactivate())
    return;
}

void ArModeBumps::help()
{
  unsigned int i;
  ArLog::log(ArLog::Terse, "Bumps mode will display whether bumpers are triggered or not...");
  ArLog::log(ArLog::Terse, "keep in mind it is assuming you have a full bump ring... so you should");
  ArLog::log(ArLog::Terse, "ignore readings for where there aren't bumpers.");
  ArLog::log(ArLog::Terse, "Bumper readings:");
  for (i = 0; i < myRobot->getNumFrontBumpers(); i++)
  {
    printf("%6d", i + 1);
  }
  printf(" |");
  for (i = 0; i < myRobot->getNumRearBumpers(); i++)
  {
    printf("%6d", i + 1);
  }
  printf("\n");
}

void ArModeBumps::userTask()
{
  unsigned int i;
  int val;
  int bit;
  if (myRobot == NULL)
    return;
  printf("\r");
  val = ((myRobot->getStallValue() & 0xff00) >> 8);
  for (i = 0, bit = 2; i < myRobot->getNumFrontBumpers(); i++, bit *= 2)
  {
    if (val & bit)
      printf("%6s", "trig");
    else
      printf("%6s", "clear");
  }
  printf(" |");
  val = ((myRobot->getStallValue() & 0xff));
  for (i = 0, bit = 2; i < myRobot->getNumRearBumpers(); i++, bit *= 2)
  {
    if (val & bit)
      printf("%6s", "trig");
    else
      printf("%6s", "clear");
  }

}

ArModePosition::ArModePosition(ArRobot *robot, const char *name, char key, char key2, ArAnalogGyro *gyro): 
  ArMode(robot, name, key, key2),
  myUpCB(this, &ArModePosition::up),
  myDownCB(this, &ArModePosition::down),
  myLeftCB(this, &ArModePosition::left),
  myRightCB(this, &ArModePosition::right),
  myStopCB(this, &ArModePosition::stop),
  myResetCB(this, &ArModePosition::reset),
  mySimResetCB(this, &ArModePosition::simreset),
  myModeCB(this, &ArModePosition::mode),
  myGyroCB(this, &ArModePosition::gyro),
  myIncDistCB(this, &ArModePosition::incDistance),
  myDecDistCB(this, &ArModePosition::decDistance)
{
  myGyro = gyro;
  myMode = MODE_BOTH;
  myModeString = "both";
  myInHeadingMode = false;
  myDistance = 1000;

  if (myGyro != NULL && !myGyro->hasNoInternalData())
    myGyroZero = myGyro->getHeading();
  myRobotZero = myRobot->getRawEncoderPose().getTh();
  myInHeadingMode = true;
  myHeading = myRobot->getTh();
}

ArModePosition::~ArModePosition()
{
  
}

void ArModePosition::activate()
{
  if (!baseActivate())
    return;

  addKeyHandler(ArKeyHandler::UP, &myUpCB);
  addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  addKeyHandler(ArKeyHandler::LEFT, &myLeftCB);  
  addKeyHandler(ArKeyHandler::RIGHT, &myRightCB);
  addKeyHandler(ArKeyHandler::SPACE, &myStopCB);
  addKeyHandler(ArKeyHandler::PAGEUP, &myIncDistCB);
  addKeyHandler(ArKeyHandler::PAGEDOWN, &myDecDistCB);
  addKeyHandler('r', &myResetCB);
  addKeyHandler('R', &mySimResetCB);
  addKeyHandler('x', &myModeCB);
  addKeyHandler('X', &myModeCB);
  addKeyHandler('z', &myGyroCB);
  addKeyHandler('Z', &myGyroCB);
}

void ArModePosition::deactivate()
{
  if (!baseDeactivate())
    return;

  remKeyHandler(&myUpCB);
  remKeyHandler(&myDownCB);
  remKeyHandler(&myLeftCB);
  remKeyHandler(&myRightCB);
  remKeyHandler(&myStopCB);
  remKeyHandler(&myResetCB);
  remKeyHandler(&myModeCB);
  remKeyHandler(&myGyroCB);
  remKeyHandler(&myIncDistCB);
  remKeyHandler(&myDecDistCB);
}

void ArModePosition::up()
{
  myRobot->move(myDistance);
  if (myInHeadingMode)
  {
    myInHeadingMode = false;
    myHeading = myRobot->getTh();
  }
}

void ArModePosition::down()
{
  myRobot->move(-myDistance);
  if (myInHeadingMode)
  {
    myInHeadingMode = false;
    myHeading = myRobot->getTh();
  }
}

void ArModePosition::incDistance()
{
  myDistance += 500;
  puts("\n");
  help();
}

void ArModePosition::decDistance()
{
  myDistance -= 500;
  if(myDistance < 500) myDistance = 500;
  puts("\n");
  help();
}

void ArModePosition::left()
{
  myRobot->setDeltaHeading(90);
  myInHeadingMode = true;
}

void ArModePosition::right()
{
  myRobot->setDeltaHeading(-90);
  myInHeadingMode = true;
}

void ArModePosition::stop()
{
  myRobot->stop();
  myInHeadingMode = true;
}

void ArModePosition::reset()
{
  myRobot->stop();
  myRobot->moveTo(ArPose(0, 0, 0));
  if (myGyro != NULL && !myGyro->hasNoInternalData())
    myGyroZero = myGyro->getHeading();
  myRobotZero = myRobot->getRawEncoderPose().getTh();
  myInHeadingMode = true;
  myHeading = myRobot->getTh();
}

void ArModePosition::simreset()
{
  ArSimUtil sim(myRobot);
  sim.setSimTruePose({0, 0, 0});
}

void ArModePosition::mode()
{
  if (myMode == MODE_BOTH)
  {
    myMode = MODE_EITHER;
    myModeString = "either";
    myInHeadingMode = true;
    myRobot->stop();
  }
  else if (myMode == MODE_EITHER)
  {
    myMode = MODE_BOTH;
    myModeString = "both";
  }
}

void ArModePosition::gyro()
{
  if (myGyro == NULL || !myGyro->haveGottenData())
    return;

  if (myGyro != NULL && myGyro->isActive())
    myGyro->deactivate();
  else if (myGyro != NULL && !myGyro->isActive() && 
	   myGyro->hasGyroOnlyMode() && !myGyro->isGyroOnlyActive())
    myGyro->activateGyroOnly();
  else if (myGyro != NULL && !myGyro->isActive())
    myGyro->activate();

  help();
}

void ArModePosition::help()
{
  ArLog::log(ArLog::Terse, "Mode is one of two values:");
  ArLog::log(ArLog::Terse, "%13s: heading and move can happen simultaneously", 
	     "both");
  ArLog::log(ArLog::Terse, "%13s: only heading or move is active (move holds heading)", "either");
  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "%13s:  forward %.1f meter(s)", "up arrow", myDistance/1000.0);
  ArLog::log(ArLog::Terse, "%13s:  backward %.1f meter(s)", "down arrow", myDistance/1000.0);
  ArLog::log(ArLog::Terse, "%13s:  increase distance by 1/2 meter", "page up");
  ArLog::log(ArLog::Terse, "%13s:  decrease distance by 1/2 meter", "page down");
  ArLog::log(ArLog::Terse, "%13s:  turn left 90 degrees", "left arrow");
  ArLog::log(ArLog::Terse, "%13s:  turn right 90 degrees", "right arrow");
  ArLog::log(ArLog::Terse, "%13s:  stop", "space bar");
  ArLog::log(ArLog::Terse, "%13s:  reset ARIA position to (0, 0, 0)", "'r'");
  ArLog::log(ArLog::Terse, "%13s:  move robot in simulator to (0, 0, 0) (but do not set odometry or ARIA position data)", "'R'");
  ArLog::log(ArLog::Terse, "%13s:  switch heading/velocity mode","'x' or 'X'");
  if (myGyro != NULL && myGyro->haveGottenData() && !myGyro->hasGyroOnlyMode())
    ArLog::log(ArLog::Terse, "%13s:  turn gyro on or off (stays this way in other modes)","'z' or 'Z'");
  if (myGyro != NULL && myGyro->haveGottenData() && myGyro->hasGyroOnlyMode())
    ArLog::log(ArLog::Terse, "%13s:  turn gyro on or off or gyro only (stays this way in other modes)","'z' or 'Z'");

  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "Position mode shows the position stats on a robot with additional teleoperation and gyro controls.");
  if (myGyro != NULL && myGyro->haveGottenData() && 
      !myGyro->hasNoInternalData())
    ArLog::log(ArLog::Terse, "%7s%7s%9s%7s%8s%7s%8s%6s%10s%10s%10s", "x", "y", "th", "comp", "volts", "mpacs", "mode", "gyro", "gyro_th", "robot_th", "raw");
  else if (myGyro != NULL && myGyro->haveGottenData() && 
	   myGyro->hasNoInternalData())
    ArLog::log(ArLog::Terse, "%7s%7s%9s%7s%8s%7s%8s%6s%10s", "x", "y", "th", "comp", "volts", "mpacs", "mode", "gyro", "raw");
  else
    ArLog::log(ArLog::Terse, "%7s%7s%9s%7s%8s%7s%8s%10s", "x", "y", "th", "comp", "volts", "mpacs", "mode", "raw");

  
}

void ArModePosition::userTask()
{
  if (myRobot == NULL)
    return;
  // if we're in either mode and not in the heading mode try to keep the
  // same heading (in heading mode its controlled by those commands)
  if (myMode == MODE_EITHER && !myInHeadingMode)
  {
    myRobot->setHeading(myHeading);
  }
  double voltage;
  if (myRobot->getRealBatteryVoltage() > 0)
    voltage = myRobot->getRealBatteryVoltage();
  else
    voltage = myRobot->getBatteryVoltage();

  std::string gyroString;
  if (myGyro == NULL)
    gyroString = "none";
  else if (myGyro->isActive())
    gyroString = "on";
  else if (myGyro->hasGyroOnlyMode() && myGyro->isGyroOnlyActive())
    gyroString = "only";
  else
    gyroString = "off";

  ArPose raw = myRobot->getRawEncoderPose();

  if (myGyro != NULL && myGyro->haveGottenData() && 
      !myGyro->hasNoInternalData())
    printf("\r%7.0f%7.0f%9.2f%7.0f%8.2f%7d%8s%6s%10.2f%10.2f %10.2f,%.2f,%.2f", 
	   myRobot->getX(),  myRobot->getY(), myRobot->getTh(), 
	   myRobot->getCompass(), voltage,
	   myRobot->getMotorPacCount(),
	   myMode == MODE_BOTH ? "both" : "either", 
	   gyroString.c_str(),
	   ArMath::subAngle(myGyro->getHeading(), myGyroZero), 
	   ArMath::subAngle(myRobot->getRawEncoderPose().getTh(),myRobotZero),
	   raw.getX(), raw.getY(), raw.getTh()
	);
  else if (myGyro != NULL && myGyro->haveGottenData() && 
      myGyro->hasNoInternalData())
    printf("\r%7.0f%7.0f%9.2f%7.0f%8.2f%7d%8s%6s%10.2f,%.2f,%.2f", 
	   myRobot->getX(),  myRobot->getY(), myRobot->getTh(), 
	   myRobot->getCompass(), voltage,
	   myRobot->getMotorPacCount(),
	   myMode == MODE_BOTH ? "both" : "either", 
	   gyroString.c_str(),
	   raw.getX(), raw.getY(), raw.getTh()

  );
  else
    printf("\r%7.0f%7.0f%9.2f%7.0f%8.2f%7d%8s%10.2f,%.2f,%.2f", myRobot->getX(), 
	   myRobot->getY(), myRobot->getTh(), myRobot->getCompass(), 
	   voltage, myRobot->getMotorPacCount(), 
	   myMode == MODE_BOTH ? "both" : "either",
	   raw.getX(), raw.getY(), raw.getTh()
    );
  fflush(stdout);
}

ArModeIO::ArModeIO(ArRobot *robot, const char *name, char key, char key2): 
  ArMode(robot, name, key, key2),
  myTog1CB(this, &ArModeIO::toggleOutput, 1),
  myTog2CB(this, &ArModeIO::toggleOutput, 2),
  myTog3CB(this, &ArModeIO::toggleOutput, 3),
  myTog4CB(this, &ArModeIO::toggleOutput, 4),
  myTog5CB(this, &ArModeIO::toggleOutput, 5),
  myTog6CB(this, &ArModeIO::toggleOutput, 6),
  myTog7CB(this, &ArModeIO::toggleOutput, 7),
  myTog8CB(this, &ArModeIO::toggleOutput, 8)
{
}

ArModeIO::~ArModeIO()
{
  
}

void ArModeIO::activate()
{
  if (!baseActivate())
    return;
  if (myRobot == NULL)
    return;
  myRobot->comInt(ArCommands::IOREQUEST, 2);
  //myOutput[0] = '\0';
  myLastPacketTime = myRobot->getIOPacketTime();
  addKeyHandler('1', &myTog1CB);
  addKeyHandler('2', &myTog2CB);
  addKeyHandler('3', &myTog3CB);
  addKeyHandler('4', &myTog4CB);
  addKeyHandler('5', &myTog5CB);
  addKeyHandler('6', &myTog6CB);
  addKeyHandler('7', &myTog7CB);
  addKeyHandler('8', &myTog8CB);
}

void ArModeIO::deactivate()
{
  if (!baseDeactivate())
    return;
  if (myRobot == NULL)
    return;
  myRobot->comInt(ArCommands::IOREQUEST, 0);
  remKeyHandler(&myTog1CB);
  remKeyHandler(&myTog2CB);
  remKeyHandler(&myTog3CB);
  remKeyHandler(&myTog4CB);
  remKeyHandler(&myTog5CB);
  remKeyHandler(&myTog6CB);
  remKeyHandler(&myTog7CB);
  remKeyHandler(&myTog8CB);
}

void ArModeIO::help()
{
  ArLog::log(ArLog::Terse, 
	     "IO mode shows the IO (digin, digout, a/d) from the robot.");
  //myExplanationReady = false;
  myExplained = false;
}

void ArModeIO::toggleOutput(int which)
{
  printf("toggling output %d. Current output is %s\n", which, byte_as_bitstring(myRobot->getDigOut()).c_str());
  uint8_t mask = (1 << (which-1));
  uint8_t bits = ~(myRobot->getDigOut());
  printf("-> DIGOUT %s %s\n", byte_as_bitstring(mask).c_str(), byte_as_bitstring(bits).c_str());
  myRobot->com2Bytes(ArCommands::DIGOUT, mask, bits);
}

void ArModeIO::userTask()
{
  int num;
  int i, j;
  //unsigned int value;
  int bit;
  //const int label_size = 12; // size of "fault_flags", the longest label. TODO calculate this automatically
  //char label[label_size];
  //myOutput[0] = '\0';
  
  std::stringstream myOutput;

  //if (myLastPacketTime.mSecSince(myRobot->getIOPacketTime()) == 0)
  //  return;

  //if (!myExplanationReady)  // flag ensures this is done only on first call to userTask()
    // reset stringstream with newly constructed temporary:
    //std::swap(std::stringstream(), myExplanation);
  //  myExplanation[0] = '\0';

 int value = myRobot->getFlags();
  if (!myExplanationReady)
  {
    //snprintf(label, label_size, "flags");
    //snprintf(myExplanation, OutputBuffLen, "%s%17s  ", myExplanation, label);
    myExplanation << std::setw(17) << "flags"; // 17 is 8 characters for bits + space + 8 bits
  }
  for (j = 0, bit = 1; j < 16; ++j, bit *= 2)
  {
    if (j == 8)
      myOutput << ' ';
      //snprintf(myOutput, OutputBuffLen, "%s ", myOutput);
    if (value & bit)
      //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
      myOutput << '1';
    else
      //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
      myOutput << '0';
  }
  //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
  myOutput << "  ";

  if (myRobot->hasFaultFlags())
  {
    value = myRobot->getFaultFlags();
    if (!myExplanationReady)
    {
      //snprintf(label, label_size, "fault_flags");
      //snprintf(myExplanation, OutputBuffLen, "%s%17s  ", myExplanation, label);
      myExplanation << std::setw(19) << "fault_flags"; // 19 is two spaces from previous flags separator + 8 chars for bits + space + 8 bits
    }
    for (j = 0, bit = 1; j < 16; ++j, bit *= 2)
    {
      if (j == 8)
        //snprintf(myOutput, OutputBuffLen, "%s ", myOutput);
        myOutput << ' ' ;
      if (value & bit)
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
        myOutput << '1';
      else
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
        myOutput << '0';
    }
    //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
    myOutput << "  ";
  }

  num = myRobot->getIODigInSize();
  for (i = 0; i < num; ++i)
  {
    value = myRobot->getIODigIn(i);
    if (!myExplanationReady)
    {
      //snprintf(label, label_size, "digin%d", i);
      //snprintf(myExplanation, OutputBuffLen, "%s%8s  ", myExplanation, label);
      myExplanation << "  digin" << std::setw(2) << i;
    }
    for (j = 0, bit = 1; j < 8; ++j, bit *= 2)
    {
      if (value & bit)
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
        myOutput << '1';
      else
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
        myOutput << '0';
    }
    //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
    myOutput << "  ";
  }
  if(num == 0) // use default Pioneer IO from SIP only if no IODigIns
  {
    value = myRobot->getDigIn();
    if(!myExplanationReady)
    {
      //snprintf(label, label_size, "digin");
      //snprintf(myExplanation, OutputBuffLen, "%s%8s  ", myExplanation, label);
      myExplanation << std::setw(10) << "  digin";
    }
    for (j = 0, bit = 1; j < 8; ++j, bit *= 2)
    {
      if (value & bit)
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
        myOutput << '1';
      else
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
        myOutput << '0';
    }
    //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
    myOutput << "  ";
  }


  num = myRobot->getIODigOutSize();
  for (i = 0; i < num; ++i)
  {
    value = myRobot->getIODigOut(i);
    if (!myExplanationReady)
    {
      //snprintf(label, label_size, "digout%d", i);
      //snprintf(myExplanation, OutputBuffLen, "%s%8s  ", myExplanation, label);
      myExplanation << std::setw(6) << "  digout" << std::setw(2) << i;
    }
    for (j = 0, bit = 1; j < 8; ++j, bit *= 2)
    {
      if (value & bit)
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
        myOutput << '1';
      else
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
        myOutput << '0';
    }
    //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
    //myOutput << "  ";
  }
  if(num == 0) // use default Pioneer IO from SIP only if no IODigIns
  {
    value = myRobot->getDigOut();
    if(!myExplanationReady)
    {
      //snprintf(label, label_size, "digout");
      //snprintf(myExplanation, OutputBuffLen, "%s%8s  ", myExplanation, label);
      myExplanation << std::setw(10) << "  digout";
    }
    for (j = 0, bit = 1; j < 8; ++j, bit *= 2)
    {
      if (value & bit)
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 1);
        myOutput << '1';
      else
        //snprintf(myOutput, OutputBuffLen, "%s%d", myOutput, 0);
        myOutput << '0';
    }
    //snprintf(myOutput, OutputBuffLen, "%s  ", myOutput);
    // myOutput << "  ";
  }

  num = myRobot->getIOAnalogSize();
  for (i = 0; i < num; ++i)
  {
    if (!myExplanationReady)
    {
      //snprintf(label, label_size, "a/d%d", i);
      //snprintf(myExplanation, OutputBuffLen, "%s%6s", myExplanation, label);
      myExplanation << std::setw(0) << "  a/d" << std::setw(3) << i;
    }
    
    /*
      int ad = myRobot->getIOAnalog(i);
      double adVal;
    ad &= 0xfff;
    adVal = ad * .0048828;
    snprintf(myOutput, OutputBuffLen, "%s%6.2f", myOutput,adVal);
    */
    //snprintf(myOutput, OutputBuffLen, "%s%6.2f", myOutput, myRobot->getIOAnalogVoltage(i));
    myOutput << std::setiosflags(std::ios::fixed) << std::setw(6) << std::setprecision(2) << myRobot->getIOAnalogVoltage(i);
  }

  if (!myExplained)
  {
    printf("Robot has %d input bits, %d output bits, %d analog io. Robot %s have fault flags.\n",
      myRobot->getIODigInSize(),
      myRobot->getIODigOutSize(),
      myRobot->getIOAnalogSize(),
      myRobot->hasFaultFlags() ? "does" : "does not"
    );
    std::cout << '\n' << myExplanation.str() << '\n';
    //printf("\n%s\n", myExplanation);
    myExplained = true;
    myExplanationReady = true;
  }

  //printf("\r%s", myOutput);
  std::cout << '\r' << myOutput.str() << std::flush;
  //fflush(stdout);
}

ArModeLaser::ArModeLaser(ArRobot *robot, const char *name, 
				  char key, char key2) :
  ArMode(robot, name, key, key2),
  myTogMiddleCB(this, &ArModeLaser::togMiddle)
{
  myPrintMiddle = false;

  ArLaser *laser;
  int i;
  for (i = 1; i <= 10; i++)
  {
    if ((laser = myRobot->findLaser(i)) != NULL)
    {
      myLaserCallbacks[i] = new ArFunctor1C<ArModeLaser, 
      int>(this, &ArModeLaser::switchToLaser, i),
      myLasers[i] = laser;
    }
  }
  
  myLaser = NULL;
  myState = STATE_UNINITED;
}

ArModeLaser::~ArModeLaser()
{
}

void ArModeLaser::activate()
{
  // this is here because there needs to be the laser set up for the
  // help to work right
  std::map<int, ArLaser *>::iterator it;
  if (myLaser == NULL)
  {
    if ((it = myLasers.begin()) == myLasers.end())
    {
      ArLog::log(ArLog::Normal, "Laser mode tried to activate, but has no lasers");
    }
    else
    {
      myLaser = (*it).second;
      myLaserNumber = (*it).first;
    }
  }


  bool alreadyActive = false;

  if (ourActiveMode == this)
    alreadyActive = true;

  if (!alreadyActive && !baseActivate())
    return;

  if (myRobot == NULL)
  {
    ArLog::log(ArLog::Verbose, "Laser mode activated but there is no robot.");
    return;
  }

  if (myLaser == NULL)
  {
    ArLog::log(ArLog::Verbose, "Laser mode activated but there are no lasers.");
	return;
  }

  if (!alreadyActive)
  {
    
    addKeyHandler('z', &myTogMiddleCB);
    addKeyHandler('Z', &myTogMiddleCB);
    
    std::map<int, ArFunctor1C<ArModeLaser, int> *>::iterator kIt;
    for (kIt = myLaserCallbacks.begin(); kIt != myLaserCallbacks.end(); kIt++)
    {
      if ((*kIt).first >= 1 || (*kIt).first <= 9)
	addKeyHandler('0' + (*kIt).first, (*kIt).second);
    }   
  }

  if (myState == STATE_UNINITED)
  {
    myLaser->lockDevice();
    if (myLaser->isConnected())
    {
      ArLog::log(ArLog::Verbose, 
		 "\nArModeLaser using already existing and connected laser.");
      myState = STATE_CONNECTED;
    }
    else if (myLaser->isTryingToConnect())
    {
      ArLog::log(ArLog::Terse, "\nArModeLaser already connecting to %s.",
		 myLaser->getName());
    }
    else
    {
      ArLog::log(ArLog::Terse,
		 "\nArModeLaser is connecting to %s.",
		 myLaser->getName());
      myLaser->asyncConnect();
      myState = STATE_CONNECTING;
    }
    myLaser->unlockDevice();
  } 
}

void ArModeLaser::deactivate()
{
  if (!baseDeactivate())
    return;

  remKeyHandler(&myTogMiddleCB);
  
  std::map<int, ArFunctor1C<ArModeLaser, int> *>::iterator it;
  for (it = myLaserCallbacks.begin(); it != myLaserCallbacks.end(); it++)
  {
    remKeyHandler((*it).second);
  }   
}

void ArModeLaser::help()
{
  if (myLaser == NULL)
  {
    ArLog::log(ArLog::Terse, 
	       "There are no lasers, this mode cannot do anything");
    return;
  }

  ArLog::log(ArLog::Terse, 
	     "Laser mode connects to a laser, or uses a previously established connection.");
  ArLog::log(ArLog::Terse,
	     "Laser mode then displays the closest and furthest reading from the laser.");
  ArLog::log(ArLog::Terse, "%13s:  toggle between far reading and middle reading with reflectivity", "'z' or 'Z'");

  std::map<int, ArFunctor1C<ArModeLaser, int> *>::iterator it;
  for (it = myLaserCallbacks.begin(); it != myLaserCallbacks.end(); it++)
  {
    ArLog::log(ArLog::Terse, "%13d:  %s", (*it).first, 
	       myLasers[(*it).first]->getName());
  }   
}


void ArModeLaser::userTask()
{
  double dist = HUGE_VAL, angle = -1;
  int reflec = -1;
  double midDist = HUGE_VAL, midAngle = -1;
  int midReflec = -1;
  double farDist = -HUGE_VAL, farAngle = -1;

  if (myRobot == NULL || myLaser == NULL)
    return;


  if (myState == STATE_CONNECTED && !myPrintMiddle)
  {
    const std::list<ArPoseWithTime *> *readings;
    std::list<ArPoseWithTime *>::const_iterator it;
    bool found = false;
  
    myLaser->lockDevice();
    if (!myLaser->isConnected())
    {
      ArLog::log(ArLog::Terse, "\n\nLaser mode lost connection to the laser.");
      ArLog::log(ArLog::Terse, "Select that laser or laser mode again to try reconnecting to the laser.\n");
      myState = STATE_UNINITED;
    }
    dist = myLaser->currentReadingPolar(-90, 90, &angle);
    if (dist < myLaser->getMaxRange())
      printf("\rClose: %8.0fmm %5.1f deg   ", dist, angle);
    else
      printf("\rNo close reading.         ");

    readings = myLaser->getCurrentBufferPtr();
    for (it = readings->begin(), found = false; it != readings->end(); it++)
    {
      dist = myRobot->findDistanceTo(*(*it));
      angle = myRobot->findDeltaHeadingTo(*(*it));
      if (!found || dist > farDist)
      {
	found = true;
	farDist = dist;
	farAngle = angle;
      }
    }
    if (found)
      printf("Far: %8.0fmm %5.1f deg", 
	     farDist, farAngle);
    else
      printf("No far reading found");
    printf("         %lu readings   ", readings->size());
    myLaser->unlockDevice();
    fflush(stdout);
  }
  else if (myState == STATE_CONNECTED && myPrintMiddle)
  {
    const std::list<ArSensorReading *> *rawReadings;
    std::list<ArSensorReading *>::const_iterator rawIt;
    myLaser->lockDevice();
    if (!myLaser->isConnected())
    {
      ArLog::log(ArLog::Terse, "\n\nLaser mode lost connection to the laser.");
      ArLog::log(ArLog::Terse, "Switch out of this mode and back if you want to try reconnecting to the laser.\n");
      myState = STATE_UNINITED;
    }
    rawReadings = myLaser->getRawReadings();
    int middleReading = rawReadings->size() / 2;
    if (rawReadings->begin() != rawReadings->end())
    {
      int i;
      for (rawIt = rawReadings->begin(), i = 0; 
	   rawIt != rawReadings->end(); 
	   rawIt++, i++)
      {
	if ((*rawIt)->getIgnoreThisReading())
	  continue;
	if (rawIt == rawReadings->begin() || 
	    (*rawIt)->getRange() < dist)
	{
	  dist = (*rawIt)->getRange();
	  angle = (*rawIt)->getSensorTh();
	  reflec = (*rawIt)->getExtraInt();
	}
	if (i == middleReading)
	{
	  midDist = (*rawIt)->getRange();
	  midAngle = (*rawIt)->getSensorTh();
	  midReflec = (*rawIt)->getExtraInt();
	}
      }
      printf(
      "\rClose: %8.0fmm %5.1f deg %d refl          Middle: %8.0fmm %5.1fdeg, %d refl", 
	      dist, angle, reflec, midDist, midAngle, midReflec);
    }
    else
      printf("\rNo readings");
    myLaser->unlockDevice(); 
  }
  else if (myState == STATE_CONNECTING)
  {
    myLaser->lockDevice();
    if (myLaser->isConnected())
    {
      ArLog::log(ArLog::Terse, "\nLaser mode has connected to the laser.\n");
      myState = STATE_CONNECTED;
    }
    else if (!myLaser->isTryingToConnect())
    {
      ArLog::log(ArLog::Terse, "\nLaser mode failed to connect to the laser.\n");
      ArLog::log(ArLog::Terse, 
		 "Switch out of this mode and back to try reconnecting.\n");
      myState = STATE_UNINITED;
    }
    myLaser->unlockDevice();
  }
}


void ArModeLaser::togMiddle()
{
  myPrintMiddle = !myPrintMiddle;
}

void ArModeLaser::switchToLaser(int laserNumber)
{
  if (laserNumber == myLaserNumber && myLaser->isConnected())
  {
    ArLog::log(ArLog::Verbose, 
	       "ArModeLaser::switchToLaser: Already on laser %s", myLaser->getName());
    return;
  }

  std::map<int, ArLaser *>::iterator it;
  if ((it = myLasers.find(laserNumber)) == myLasers.end())
  {
    ArLog::log(ArLog::Normal, "ArModeLaser::switchToLaser: told to switch to laser %d but that laser does not exist"); 
    return;
  }
  myLaser = (*it).second;
  ArLog::log(ArLog::Normal, "\r\n\nSwitching to laser %s\n", 
	     myLaser->getName()); 
  myState = STATE_UNINITED;
  myLaserNumber = laserNumber;

  activate();
}

#if 0
/**
  @param robot ArRobot instance to be associate with
  @param name name of this mode
  @param key keyboard key that activates this mode
  @param key2 another keyboard key that activates this mode
  @param acts ArACTS_1_2 instance to use. If not given, then an internally
maintained instance is created by ArModeActs.
 **/
ArModeActs::ArModeActs(ArRobot *robot, const char *name, char key, 
				char key2, ArACTS_1_2 *acts): 
  ArMode(robot, name, key, key2),
  myChannel1CB(this, &ArModeActs::channel1),
  myChannel2CB(this, &ArModeActs::channel2),
  myChannel3CB(this, &ArModeActs::channel3),
  myChannel4CB(this, &ArModeActs::channel4),
  myChannel5CB(this, &ArModeActs::channel5),
  myChannel6CB(this, &ArModeActs::channel6),
  myChannel7CB(this, &ArModeActs::channel7),
  myChannel8CB(this, &ArModeActs::channel8),
  myStopCB(this, &ArModeActs::stop),
  myStartCB(this, &ArModeActs::start),
  myToggleAcquireCB(this, &ArModeActs::toggleAcquire)
{
  if (acts != NULL)
    myActs = acts;
  else
    myActs = new ArACTS_1_2;
  myRobot = robot;
  myActs->openPort(myRobot);
  myGroup = new ArActionGroupColorFollow(myRobot, myActs, camera);
  myGroup->deactivate();
}

// Destructor
ArModeActs::~ArModeActs()
{
  
}

// Activate the mode
void ArModeActs::activate()
{
  // Activate the group
  if (!baseActivate())
    return;
  myGroup->activateExclusive();
  
  // Add key handlers for keyboard input
  addKeyHandler(ArKeyHandler::SPACE, &myStopCB);
  addKeyHandler('z', &myStartCB);
  addKeyHandler('Z', &myStartCB);
  addKeyHandler('x', &myToggleAcquireCB);
  addKeyHandler('X', &myToggleAcquireCB);
  addKeyHandler('1', &myChannel1CB);
  addKeyHandler('2', &myChannel2CB);
  addKeyHandler('3', &myChannel3CB);
  addKeyHandler('4', &myChannel4CB);
  addKeyHandler('5', &myChannel5CB);
  addKeyHandler('6', &myChannel6CB);
  addKeyHandler('7', &myChannel7CB);
  addKeyHandler('8', &myChannel8CB);

  // Set the camera
  camera = myRobot->getPTZ();
  
  // Tell us whether we are connected to ACTS or not
  if(myActs->isConnected())
    { 
      printf("\nConnected to ACTS.\n");
    }
  else printf("\nNot connected to ACTS.\n");

  // Tell us whether a camera is defined or not
  if(camera != NULL)
    {
      printf("\nCamera defined.\n\n");
      myGroup->setCamera(camera);
    }
  else
    {  
      printf("\nNo camera defined.\n");
      printf("The robot will not tilt its camera up or down until\n");
      printf("a camera has been defined in camera mode ('c' or 'C').\n\n");
    }
}

// Deactivate the group
void ArModeActs::deactivate()
{
  if (!baseDeactivate())
    return;

  // Remove the key handlers
  remKeyHandler(&myStopCB);
  remKeyHandler(&myStartCB);
  remKeyHandler(&myToggleAcquireCB);
  remKeyHandler(&myChannel1CB);
  remKeyHandler(&myChannel2CB);
  remKeyHandler(&myChannel3CB);
  remKeyHandler(&myChannel4CB);
  remKeyHandler(&myChannel5CB);
  remKeyHandler(&myChannel6CB);
  remKeyHandler(&myChannel7CB);
  remKeyHandler(&myChannel8CB);

  myGroup->deactivate();
}

// Display the available commands
void ArModeActs::help()
{
  ArLog::log(ArLog::Terse, 
	   "ACTS mode will drive the robot in an attempt to follow a color blob.\n");

  ArLog::log(ArLog::Terse, "%20s:  Pick a channel",     "1 - 8    ");
  ArLog::log(ArLog::Terse, "%20s:  toggle acquire mode", "'x' or 'X'");
  ArLog::log(ArLog::Terse, "%20s:  start movement",     "'z' or 'Z'");
  ArLog::log(ArLog::Terse, "%20s:  stop movement",      "space bar");
  ArLog::log(ArLog::Terse, "");
  
}

// Display data about this mode
void ArModeActs::userTask()
{
  int myChannel;

  const char *acquire;
  const char *move;
  const char *blob;

  myChannel = myGroup->getChannel();
  if(myGroup->getAcquire()) acquire = "actively acquiring";
  else acquire = "passively acquiring";
  
  if(myGroup->getMovement()) move = "movement on";
  else move = "movement off";

  if(myGroup->getBlob()) blob = "blob in sight";
  else blob = "no blob in sight";

  printf("\r Channel: %d  %15s %25s %20s", myChannel, move, acquire, blob);
  fflush(stdout);
}

// The channels
void ArModeActs::channel1()
{
  myGroup->setChannel(1);
}

void ArModeActs::channel2()
{
  myGroup->setChannel(2);
}

void ArModeActs::channel3()
{
  myGroup->setChannel(3);
}

void ArModeActs::channel4()
{
  myGroup->setChannel(4);
}

void ArModeActs::channel5()
{
  myGroup->setChannel(5);
}

void ArModeActs::channel6()
{
  myGroup->setChannel(6);
}

void ArModeActs::channel7()
{
  myGroup->setChannel(7);
}

void ArModeActs::channel8()
{
  myGroup->setChannel(8);
}

// Stop the robot from moving
void ArModeActs::stop()
{
  myGroup->stopMovement();
}

// Allow the robot to move
void ArModeActs::start()
{
  myGroup->startMovement();
}

// Toggle whether or not the robot is allowed
// to aquire anything
void ArModeActs::toggleAcquire()
{
  if(myGroup->getAcquire())
    myGroup->setAcquire(false);
  else myGroup->setAcquire(true);

}
#endif

ArModeCommand::ArModeCommand(ArRobot *robot, const char *name, char key, char key2): 
  ArMode(robot, name, key, key2),
  my0CB(this, &ArModeCommand::addChar, '0'),
  my1CB(this, &ArModeCommand::addChar, '1'),
  my2CB(this, &ArModeCommand::addChar, '2'),
  my3CB(this, &ArModeCommand::addChar, '3'),
  my4CB(this, &ArModeCommand::addChar, '4'),
  my5CB(this, &ArModeCommand::addChar, '5'),
  my6CB(this, &ArModeCommand::addChar, '6'),
  my7CB(this, &ArModeCommand::addChar, '7'),
  my8CB(this, &ArModeCommand::addChar, '8'),
  my9CB(this, &ArModeCommand::addChar, '9'),
  myMinusCB(this, &ArModeCommand::addChar, '-'),
  myBackspaceCB(this, &ArModeCommand::addChar, ArKeyHandler::BACKSPACE),
  mySpaceCB(this, &ArModeCommand::addChar, ArKeyHandler::SPACE),
  myEnterCB(this, &ArModeCommand::finishParsing)

{
  reset(false);
}

ArModeCommand::~ArModeCommand()
{
  
}

void ArModeCommand::activate()
{
  reset(false);
  if (!baseActivate())
    return;
  myRobot->stopStateReflection();
  takeKeys();
  reset(true);
}

void ArModeCommand::deactivate()
{
  if (!baseDeactivate())
    return;
  giveUpKeys();
}

void ArModeCommand::help()
{
  
  ArLog::log(ArLog::Terse, "Command mode has three ways to send commands");
  ArLog::log(ArLog::Terse, "%-30s: Sends com(<command>)", "<command>");
  ArLog::log(ArLog::Terse, "%-30s: Sends comInt(<command>, <integer>)", "<command> <integer>");
  ArLog::log(ArLog::Terse, "%-30s: Sends com2Bytes(<command>, <byte1>, <byte2>)", "<command> <byte1> <byte2>");
}

void ArModeCommand::addChar(int ch)
{
  if (ch < '0' && ch > '9' && ch != '-' && ch != ArKeyHandler::BACKSPACE && 
      ch != ArKeyHandler::SPACE)
  {
    ArLog::log(ArLog::Terse, "Something horribly wrong in command mode since number is < 0 || > 9 (it is the value %d)", ch);
    return;
  }

  size_t size = sizeof(myCommandString);
  size_t len = strlen(myCommandString);

  if (ch == ArKeyHandler::BACKSPACE)
  {
    // don't overrun backwards
    if (len < 1)
      return;
    myCommandString[len-1] = '\0';
    printf("\r> %s  \r> %s", myCommandString, myCommandString);
    return;
  }
  if (ch == ArKeyHandler::SPACE)
  {
    // if we're at the start or have a space or - just return
    if (len < 1 || myCommandString[len-1] == ' ' || 
	myCommandString[len-1] == '-')
      return;
    myCommandString[len] = ' ';
    myCommandString[len+1] = '\0';
    printf(" ");
    return;
  }
  if (ch == '-')
  {
    // make sure it isn't the command trying to be negated or that its the start of the byte
    if (len < 1 || myCommandString[len-1] != ' ')
      return;
    printf("%c", '-');
    myCommandString[len] = '-';
    myCommandString[len+1] = '\0';
    return;
  }
  if (len + 1 >= size)
  {
    printf("\n");
    ArLog::log(ArLog::Terse, "Command is too long, abandoning command");
    reset();
    return;
  }
  else
  {
    printf("%c", ch);
    myCommandString[len] = ch;
    myCommandString[len+1] = '\0';
    return;
  }
}

void ArModeCommand::finishParsing()
{
  
  ArArgumentBuilder builder;
  builder.addPlain(myCommandString);
  int command;
  int int1;
  int int2;

  if (myCommandString[0] == '\0')
    return;

  printf("\n");
  if (builder.getArgc() == 0)
  {
    ArLog::log(ArLog::Terse, "Syntax error, no arguments.");
  }
  if (builder.getArgc() == 1)
  {
    command = builder.getArgInt(0);
    if (command < 0 || command > 255 || !builder.isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "Invalid command, must be an integer between 0 and 255");
      reset();
      return;
    }
    else
    {
      ArLog::log(ArLog::Terse, "com(%d)", command);
      myRobot->com(command);
      reset();
      return;
    }
  }
  else if (builder.getArgc() == 2)
  {
    command = builder.getArgInt(0);
    int1 = builder.getArgInt(1);
    if (command < 0 || command > 255 || !builder.isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "Invalid command, must be an integer between 0 and 255");
      reset();
      return;
    }
    else if (int1 < -32767 || int1 > 32767 || !builder.isArgInt(1))
    {
      ArLog::log(ArLog::Terse, 
	 "Invalid integer, must be an integer between -32767 and 32767");
      reset();
      return;
    }
    else
    {
      ArLog::log(ArLog::Terse, "comInt(%d, %d)", command,
		 int1);
      myRobot->comInt(command, int1);
      reset();
      return;
    }
  }
  else if (builder.getArgc() == 3)
  {
    command = builder.getArgInt(0);
    int1 = builder.getArgInt(1);
    int2 = builder.getArgInt(2);
    if (command < 0 || command > 255 || !builder.isArgInt(0))
    {
      ArLog::log(ArLog::Terse, 
		 "Invalid command, must be between 0 and 255");
      reset();
      return;
    }
    else if (int1 < -128 || int1 > 255 || !builder.isArgInt(1))
    {
      ArLog::log(ArLog::Terse, 
	 "Invalid byte1, must be an integer between -128 and 127, or between 0 and 255");
      reset();
      return;
    }
    else if (int2 < -128 || int2 > 255 || !builder.isArgInt(2))
    {
      ArLog::log(ArLog::Terse, 
	 "Invalid byte2, must be an integer between -128 and 127, or between 0 and 255");
      reset();
      return;
    }
    else
    {
      ArLog::log(ArLog::Terse, 
		 "com2Bytes(%d, %d, %d)", 
		 command, int1, int2);
      myRobot->com2Bytes(command, int1, int2);
      reset();
      return;
    }
  }
  else
  {
    ArLog::log(ArLog::Terse, "Syntax error, too many arguments");
    reset();
    return;
  }
}

void ArModeCommand::reset(bool print)
{
  myCommandString[0] = '\0';
  if (print)
  {
    ArLog::log(ArLog::Terse, "");
    printf("> ");
  }
}

void ArModeCommand::takeKeys()
{
  addKeyHandler('0', &my0CB);
  addKeyHandler('1', &my1CB);
  addKeyHandler('2', &my2CB);
  addKeyHandler('3', &my3CB);
  addKeyHandler('4', &my4CB);
  addKeyHandler('5', &my5CB);
  addKeyHandler('6', &my6CB);
  addKeyHandler('7', &my7CB);
  addKeyHandler('8', &my8CB);
  addKeyHandler('9', &my9CB);
  addKeyHandler('-', &myMinusCB);
  addKeyHandler(ArKeyHandler::BACKSPACE, &myBackspaceCB);
  addKeyHandler(ArKeyHandler::ENTER, &myEnterCB);
  addKeyHandler(ArKeyHandler::SPACE, &mySpaceCB);
}

void ArModeCommand::giveUpKeys()
{
  remKeyHandler(&my0CB);
  remKeyHandler(&my1CB);
  remKeyHandler(&my2CB);
  remKeyHandler(&my3CB);
  remKeyHandler(&my4CB);
  remKeyHandler(&my5CB);
  remKeyHandler(&my6CB);
  remKeyHandler(&my7CB);
  remKeyHandler(&my8CB);
  remKeyHandler(&my9CB);
  remKeyHandler(&myBackspaceCB);
  remKeyHandler(&myMinusCB);
  remKeyHandler(&myEnterCB);
  remKeyHandler(&mySpaceCB);
}


#if 0
/**
  @param robot ArRobot instance to be associate with
  @param name name of this mode
  @param key keyboard key that activates this mode
  @param key2 another keyboard key that activates this mode
   @param tcm2 if a tcm2 class is passed in it'll use that instance
   otherwise it'll make its own ArTCMCompassRobot instance.
**/

ArModeTCM2::ArModeTCM2(ArRobot *robot, const char *name, char key, char key2, ArTCM2 *tcm2): 
  ArMode(robot, name, key, key2)
{
  if (tcm2 != NULL)
    myTCM2 = tcm2;
  else
    myTCM2 = new ArTCMCompassRobot(robot);

  myOffCB = new ArFunctorC<ArTCM2>(myTCM2, &ArTCM2::commandOff);
  myCompassCB = new ArFunctorC<ArTCM2>(myTCM2, &ArTCM2::commandJustCompass);
  myOnePacketCB = new ArFunctorC<ArTCM2>(myTCM2, &ArTCM2::commandOnePacket);
  myContinuousPacketsCB = new ArFunctorC<ArTCM2>(
	  myTCM2, &ArTCM2::commandContinuousPackets);
  myUserCalibrationCB = new ArFunctorC<ArTCM2>(
	  myTCM2, &ArTCM2::commandUserCalibration);
  myAutoCalibrationCB = new ArFunctorC<ArTCM2>(
	  myTCM2, &ArTCM2::commandAutoCalibration);
  myStopCalibrationCB = new ArFunctorC<ArTCM2>(
	  myTCM2, &ArTCM2::commandStopCalibration);
  myResetCB = new ArFunctorC<ArTCM2>(
	  myTCM2, &ArTCM2::commandSoftReset);

}

ArModeTCM2::~ArModeTCM2()
{
  
}

void ArModeTCM2::activate()
{
  if (!baseActivate())
    return;
  myTCM2->commandContinuousPackets();
  addKeyHandler('0', myOffCB);
  addKeyHandler('1', myCompassCB);
  addKeyHandler('2', myOnePacketCB);
  addKeyHandler('3', myContinuousPacketsCB);
  addKeyHandler('4', myUserCalibrationCB);
  addKeyHandler('5', myAutoCalibrationCB);
  addKeyHandler('6', myStopCalibrationCB);
  addKeyHandler('7', myResetCB);
}

void ArModeTCM2::deactivate()
{
  if (!baseDeactivate())
    return;
  myTCM2->commandJustCompass();
  remKeyHandler(myOffCB);
  remKeyHandler(myCompassCB);
  remKeyHandler(myOnePacketCB);
  remKeyHandler(myContinuousPacketsCB);
  remKeyHandler(myUserCalibrationCB);
  remKeyHandler(myAutoCalibrationCB);
  remKeyHandler(myStopCalibrationCB);
  remKeyHandler(myResetCB);
}

void ArModeTCM2::help()
{
  ArLog::log(ArLog::Terse, 
	     "TCM2 mode shows the data from the TCM2 compass and lets you send the TCM2 commands");
  ArLog::log(ArLog::Terse, "%20s:  turn TCM2 off", "'0'");  
  ArLog::log(ArLog::Terse, "%20s:  just get compass readings", "'1'");  
  ArLog::log(ArLog::Terse, "%20s:  get a single set of TCM2 data", "'2'");  
  ArLog::log(ArLog::Terse, "%20s:  get continuous TCM2 data", "'3'");  
  ArLog::log(ArLog::Terse, "%20s:  start user calibration", "'4'");  
  ArLog::log(ArLog::Terse, "%20s:  start auto calibration", "'5'");  
  ArLog::log(ArLog::Terse, "%20s:  stop calibration and get a single set of data", "'6'");  
  ArLog::log(ArLog::Terse, "%20s:  soft reset of compass", "'7'");  

  printf("%6s %5s %5s %6s %6s %6s %6s %10s %4s %4s %6s %3s\n", 
	 "comp", "pitch", "roll", "magX", "magY", "magZ", "temp", "error",
	 "calH", "calV", "calM", "cnt");
}

void ArModeTCM2::userTask()
{
  printf("\r%6.1f %5.1f %5.1f %6.2f %6.2f %6.2f %6.1f 0x%08x %4.0f %4.0f %6.2f %3d", 
	 myTCM2->getCompass(), myTCM2->getPitch(), myTCM2->getRoll(), 
	 myTCM2->getXMagnetic(), myTCM2->getYMagnetic(), 
	 myTCM2->getZMagnetic(), 
	 myTCM2->getTemperature(), myTCM2->getError(), 
	 myTCM2->getCalibrationH(), myTCM2->getCalibrationV(), 
	 myTCM2->getCalibrationM(), myTCM2->getPacCount());
  fflush(stdout);

}
#endif


ArModeConfig::ArModeConfig(ArRobot *robot, const char *name, char key1, char key2) :
  ArMode(robot, name, key1, key2),
  myRobot(robot),
  myConfigPacketReader(robot, false, &myGotConfigPacketCB),
  myGotConfigPacketCB(this, &ArModeConfig::gotConfigPacket)
{
}

void ArModeConfig::help()
{
  ArLog::log(ArLog::Terse, "Robot Config mode requests a CONFIG packet from the robot and displays the result.");
}

void ArModeConfig::activate()
{
  baseActivate();  // returns false on double activate, but we want to use this signal to request another config packet, so ignore.
  if(!myConfigPacketReader.requestPacket())
    ArLog::log(ArLog::Terse, "ArModeConfig: Warning: config packet reader did not request (another) CONFIG packet.");
}

void ArModeConfig::deactivate()
{
}

void ArModeConfig::gotConfigPacket()
{
  ArLog::log(ArLog::Terse, "\nRobot CONFIG packet received:");
  myConfigPacketReader.log();
  myConfigPacketReader.logMovement();
  ArLog::log(ArLog::Terse, "Additional robot information:");
  ArLog::log(ArLog::Terse, "HasStateOfCharge %d", myRobot->haveStateOfCharge());
  ArLog::log(ArLog::Terse, "StateOfChargeLow %f", myRobot->getStateOfChargeLow());
  ArLog::log(ArLog::Terse, "StateOfChargeShutdown %f", myRobot->getStateOfChargeShutdown());
  ArLog::log(ArLog::Terse, "HasFaultFlags %d", myRobot->hasFaultFlags());
  ArLog::log(ArLog::Terse, "HasTableIR %d", myRobot->hasTableSensingIR());
  ArLog::log(ArLog::Terse, "NumSonar (rec'd) %d", myRobot->getNumSonar());
  ArLog::log(ArLog::Terse, "HasTemperature (rec'd) %d", myRobot->hasTemperature());
  ArLog::log(ArLog::Terse, "HasSettableVelMaxes %d", myRobot->hasSettableVelMaxes());
  ArLog::log(ArLog::Terse, "HasSettableAccsDecs %d", myRobot->hasSettableAccsDecs());
  ArLog::log(ArLog::Terse, "HasLatVel %d", myRobot->hasLatVel());
  ArLog::log(ArLog::Terse, "HasMoveCommand %d", myRobot->getRobotParams()->hasMoveCommand());
  ArLog::log(ArLog::Terse, "Radius %f Width %f Length %f LengthFront %f LengthRear %f Diagonal %f", 
    myRobot->getRobotRadius(),
    myRobot->getRobotWidth(),
    myRobot->getRobotLength(),
    myRobot->getRobotLengthFront(),
    myRobot->getRobotLengthRear(),
    myRobot->getRobotDiagonal() 
  );
}


ArModeRobotStatus::ArModeRobotStatus(ArRobot *robot, const char *name, char key1, char key2) :
  ArMode(robot, name, key1, key2),
  myRobot(robot),
  myDebugMessageCB(this, &ArModeRobotStatus::handleDebugMessage),
  mySafetyStateCB(this, &ArModeRobotStatus::handleSafetyStatePacket),
  mySafetyWarningCB(this, &ArModeRobotStatus::handleSafetyWarningPacket),
  myBatteryShutdown(false),
  myToggleShutdownCB(this, &ArModeRobotStatus::toggleShutdown)
{
}

void ArModeRobotStatus::help()
{
  puts("Robot diagnostic flags mode prints the current state of the robot's error and diagnostic flags."); 
  puts("Additional debug and status information will also be requested from the robot and logged if received.");
  puts("");
  puts("volt:        Reported battery voltage");
  puts("soc:         Reported battery state of charge (if available)");
  puts("temp:        Reported temperature (-127 if unavailable)");
  puts("motors:      Are motors enabled?");
  puts("estop:       Is any e-stop button or mechanism engaged?");
  puts("stall:       Is left (L) or right (R) motor stalled?");
  puts("stallflags:  0=left motor stall, 1-7=front bumper segments hit, 8=right motor stall, 9-15=rear bumpers" );
  puts("sip/s:       Robot status packets (SIP) received per second. Normally 10.");
  puts("flags:       0 = motors enabled, 1-4 = sonar array enabled, 5 = estop, 7-8 = legacy IR, 9 = joystick button, 11 = high temperature ");
  puts("faults:      0=power error, 1=high temperature, 2=power error, 3=undervoltage, 4=gyro fault, 5=battery overtemperature, \n"
       "             6=battery balance needed, 7=encoder degradation warning, 8=encoder failure, 9=drive fault, 10=estop mismatch,\n"
       "             11=estop fail, 12=speed zone fault, 13=safety system fault, 14=backed up too fast, 15=joydrive warning"
  );
  puts("chargestate: Battery charging status");
  puts("chargepower: Whether battery charge power is being received by battery (if this robot can measure this)");
  puts("flags3:      0=joystick override mode enabled, 1=amp comm error, 2=silent estop, 3=laser error N, 4=RCL disabled, 5=rotation saturated");
  puts("");

  printf("Key commands:\n%13s:  wait for battery SoC to fall below 30%% (or if SoC is unavailable, for voltage to fall below 11.5V) and shut down robot power\n\n", "x");

  printFlagsHeader();
}

void ArModeRobotStatus::activate()
{
  if(baseActivate())
  {
    // only do the following on the first activate. they remain activated.
    myRobot->lock();
    myRobot->addPacketHandler(&myDebugMessageCB);
    myRobot->addPacketHandler(&mySafetyStateCB);
    myRobot->addPacketHandler(&mySafetyWarningCB);
    myRobot->unlock();
  }
  else
  {
    // error activating base mode class
    return;
  }

  addKeyHandler('x', &myToggleShutdownCB);


  if(myBatteryShutdown) puts("Will send commands to shut down robot power if state of charge is <= 30%% or battery voltage is <= 11.5/23V. Press X to cancel.");

  puts("");
  printFlagsHeader();
  printFlags();
  puts("\n");
    
    
  myRobot->lock();
  int flags = myRobot->getFlags();
  int faults = myRobot->hasFaultFlags() ? myRobot->getFaultFlags() : 0;
  int flags3 = myRobot->hasFlags3() ? myRobot->getFlags3() : 0;
  int configflags = 0;
  const ArRobotConfigPacketReader *configreader = myRobot->getOrigRobotConfig();
  if(configreader)
    configflags = configreader->getConfigFlags();
  myRobot->unlock();


  if(flags)
  {
    puts("Active Flags:");
    if(flags & ArUtil::BIT0)
      puts("\tMotors are enabled (flag 0)");
    else
      puts("\tMotors are disabled (flag 0)");
    if(flags & ArUtil::BIT5)
      puts("\tESTOP (flag 5)");
    if(flags & ArUtil::BIT9)
      puts("\tJoystick button pressed (flag 9)");
    if(flags & ArUtil::BIT11)
      puts("\tHigh temperature. (flag 11)");
  }


  if(faults)
  {
    puts("Active Fault Flags:");
    if(faults & ArUtil::BIT0)
      puts("\tPDB Laser Status Error (fault 0)");
    if(faults & ArUtil::BIT1)
      puts("\tHigh Temperature (fault 1)");
    if(faults & ArUtil::BIT2)
      puts("\tPDB Error (fault 2)");
    if(faults & ArUtil::BIT3)
      puts("\tUndervoltage/Low Battery (fault 3)");
    if(faults & ArUtil::BIT4)
      puts("\tGyro Critical Fault (fault 4)");
    if(faults & ArUtil::BIT5)
      puts("\tBattery Overtemperature (fault 5)");
    if(faults & ArUtil::BIT6)
      puts("\tBattery balance required (fault 6)");
    if(faults & ArUtil::BIT7)
      puts("\tEncoder degradation (fault 7)");
    if(faults & ArUtil::BIT8)
      puts("\tEncoder failure (fault 8)");
    if(faults & ArUtil::BIT9)
      puts("\tCritical general driving fault (fault 9)");
    if(faults & ArUtil::BIT10)
      puts("\tESTOP Mismatch Warning. One ESTOP channel may be intermittent or failing. Check connections to control panel. (ESTOP_MISMATCH_FLAG, 10)");
    if(faults & ArUtil::BIT11)
      puts("\tESTOP Safety Fault. ESTOP circuitry has failed. Motors disabled until safety system recommision or disabled. (ESTOP_SAFETY_FAULT, 11)");
    if(faults & ArUtil::BIT12)
      puts("\tLaser/speed zone failure or zone mismatch. Speed limited until safety system recommisioa or disabled.  (SPEED_ZONE_SAFETY_FAULT, 12)");
    if(faults & ArUtil::BIT13)
      puts("\tSAFETY_UNKNOWN_FAULT (fault 13)");
    if(faults & ArUtil::BIT14)
      puts("\tBacked up too fast. Reduce speed to avoid or disable safety system to allow faster reverse motion. (fault 14)");
    if(faults & ArUtil::BIT15)
      puts("\tJoydrive unsafe mode warning (fault 15)");
  }

  
  if(flags3)
  {
    puts("Active Flags3:");
    if(flags3 & ArUtil::BIT0)
      puts("\tJoystick override mode enabled (0)");
    if(flags3 & ArUtil::BIT1)
      puts("\tAmp. comm. error (1)");
    if(flags3 & ArUtil::BIT2)
      puts("\tSilent E-Stop (2)");
    if(flags3 & ArUtil::BIT3)
      puts("\tLaser safety circuit error (S300 error 'n') (3)");
    if(~flags3 & ArUtil::BIT4)
      puts("\tRotation control loop not enabled (4)");
    if(flags3 & ArUtil::BIT5)
      puts("\tRotation integrator saturated (5)");
  }
    
  
  if(configflags & ArUtil::BIT0)
  {
    puts("ConfigFlags:");
    puts("\tFirmware boot error. Robot controller bootloader detected but no firmware. (config flag 0 set)");
  }

  fflush(stdout);

  // TODO keys to enable/disable: LogMovementSent, LogMovementReceived,
  // LogVelocitiesReceived, PacketsReceivedTracking, LogSIPContents,
  // PacketsSentTracking.
  // TODO keys to start/stop WHEEL_INFO
  
  // TODO get simulator info

  myRobot->comInt(214, 1); // request state of safety systems

  // print first header line for user task refresh
  puts("");
  printFlagsHeader();
}

void ArModeRobotStatus::deactivate()
{
  remKeyHandler(&myToggleShutdownCB);
  // commented out to keep packet handlers active so we can use other modes but
  // still see debug messages
  //myRobot->remPacketHandler(&myDebugMessageCB);
  //myRobot->remPacketHandler(&mySafetyStateCB);
  //myRobot->remPacketHandler(&mySafetyWarningCB);
  if(!baseDeactivate()) return;
}

void ArModeRobotStatus::toggleShutdown()
{
  myBatteryShutdown = !myBatteryShutdown;

  if(myBatteryShutdown)
  {
    puts("Will shut down robot power when state of charge is <= 30%% or battery voltage is <= 11.5/23V.");
  }
  else
  {
    puts("Cancelled shutdown");
  }
}


void ArModeRobotStatus::userTask()
{
  printFlags();
  printf("\r");
  fflush(stdout);
  
  if(myBatteryShutdown &&
    ( (myRobot->haveStateOfCharge() && (myRobot->getStateOfCharge() <= 30)) ||
      (!myRobot->haveStateOfCharge() && (myRobot->getBatteryVoltage() <= 11.5)) )
  )
  {
    puts("stopping robot");
    myRobot->com(ArCommands::STOP);
    ArUtil::sleep(50);
    myRobot->comInt(ArCommands::ESTOP, 1);
    puts("sending (various) commands to shut down robot");

    std::map<int, ArBatteryMTX*> *batMap = myRobot->getBatteryMap();
    if(batMap)
    {
      for(std::map<int, ArBatteryMTX*>::reverse_iterator i = batMap->rbegin(); i != batMap->rend(); ++i)
      {
        puts("telling an MTX battery to shut off");
        ArBatteryMTX *b = i->second;
        if(b && b->isConnected()) b->sendEmergencyPowerOff();
      }
    }

    puts("telling mt400 to shut off");
    myRobot->com2Bytes(31, 1, 1); // mt400/patrolbot

    puts("telling seekur to recenter and power off");
    myRobot->com(119); // seekur

  }
}
  

void ArModeRobotStatus::printFlagsHeader()
{
  const char* headerfmt = 
    "%-4s "  // volts
    "%-4s "  // soc
    "%-4s "     // temp
    "%-6s "     // mot.enable?
    "%-6s "      // estop?
    "%-6s "      // stallL?
    "%-2s "      // stallR?
    "%-16s "     // stallval
    "%-5s "      // #sip/sec    
    "%-16s "     // flags
    "%-16s "     // fault flags
    "%-13s "     // chargestate
    "%-7s "     // chargerpower
    "%-32s "     // flags3
    "\n";


  printf(headerfmt,
        "volt",
        "soc",
        "temp",
        "motors",
        "estop",
        "L stall",
        "R",
        "stallval",
        "sip/s",
        "flags",
        "faults",
        "chargestate",
        "charge",
        "flags3"
    );
}

void ArModeRobotStatus::printFlags()
{
  const char* datafmt = 
    "%-03.01f "  // volts
    "%-03.02f "  // soc
    "%- 04d "     // temp
    "%-6s "     // mot.enable?
    "%-6s "      // estop?
    "%-6s "      // stallL?
    "%-3s "      // stallR?
    "%-16s "     // stallval
    "%-5d "      // #sip/sec    
    "%-16s "     // flags
    "%-16s "     // fault flags
    "%-13s "     // chargestate
    "%-7s "     // chargerpower
    "%-32s "     // flags3
  ;

  myRobot->lock();

  printf(datafmt,
    myRobot->getRealBatteryVoltage(),
    myRobot->getStateOfCharge(),
    myRobot->getTemperature(),
    myRobot->areMotorsEnabled()?"yes":"NO",
    myRobot->isEStopPressed()?"YES":"no",
    myRobot->isLeftMotorStalled()?"YES":"no",
    myRobot->isRightMotorStalled()?"YES":"no",
    int16_as_bitstring(myRobot->getStallValue()).c_str(),
    myRobot->getMotorPacCount() ,
    int16_as_bitstring(myRobot->getFlags()).c_str(),
    myRobot->hasFaultFlags() ? int16_as_bitstring(myRobot->getFaultFlags()).c_str() : "n/a",
    myRobot->getChargeStateName(),
    myRobot->isChargerPowerGood() ? "YES" : "no",
    myRobot->hasFlags3() ? int32_as_bitstring(myRobot->getFlags3()).c_str() : "n/a"
  );
  myRobot->unlock();
}

bool ArModeRobotStatus::handleDebugMessage(ArRobotPacket *pkt)
{
  if(pkt->getID() != ArCommands::MARCDEBUG) return false;
  char msg[256];
  pkt->bufToStr(msg, sizeof(msg));
  msg[255] = 0;
  ArLog::log(ArLog::Terse, "Firmware Debug Message Received: %s", msg);
  return true;
}


const char *ArModeRobotStatus::safetyStateName(int state)
{
  switch (state) {
    case 0:
      return "unknown/initial";
    case 0x10:
      return "failure";
    case 0x20: 
      return "warning";
    case 0x40:
      return "commissioned";
    case 0x50:
      return "decommissioned/disabled";
  }
  return "invalid/unknown";
}

bool ArModeRobotStatus::handleSafetyStatePacket(ArRobotPacket *p)
{
  if(p->getID() != 214) return false;
  int state = p->bufToUByte();
  int estop_state = p->bufToUByte();
  int laser_state = p->bufToUByte();
  ArLog::log(ArLog::Normal, "Safety system state: 0x%x, system0(estop)=0x%x, %s, system1(laser)=0x%x, %s\n", state, estop_state, safetyStateName(estop_state), laser_state, safetyStateName(laser_state));
  if(state == 0x10) ArLog::log(ArLog::Terse, "Warning: Safety system enabled with failure detected, robot controller will not allow motion.");
  if(state == 0x20) ArLog::log(ArLog::Terse, "Warning: Safety system enabled with warning indicated, robot controller will limit robot motion.");
  if(estop_state == 0x10) ArLog::log(ArLog::Terse, "Warning: Safety estop subsystem enabled with failure detected, robot controller will not allow motion.");
  if(estop_state == 0x20) ArLog::log(ArLog::Terse, "Warning: Safety estop subsystem enabled with warning indicated, robot controller will limit robot motion.");
  if(laser_state == 0x10) ArLog::log(ArLog::Terse, "Warning: Safety laser subsystem enabled with failure detected, robot controller will not allow motion."); 
  if(laser_state == 0x20) ArLog::log(ArLog::Terse, "Warning: Safety laser subsystem enabled with warning indicated, robot controller will limit robot motion.");
  return true;
}

bool ArModeRobotStatus::handleSafetyWarningPacket(ArRobotPacket *p)
{
  if(p->getID() != 217) return false;
  ArLog::log(ArLog::Terse, "Safety system warning received!");
  return false; // let other stuff also handle it
}
