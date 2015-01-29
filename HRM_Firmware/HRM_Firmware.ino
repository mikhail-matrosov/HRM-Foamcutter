#include <assert.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <AccelStepper.h>
#include "utility/Adafruit_PWMServoDriver.h"

#define _DBG_
void dbgPrint(String str)
{
#ifdef _DBG_
	Serial.println(str);
#endif
}
void dbgPrint(int i)
{
#ifdef _DBG_
	Serial.println(i);
#endif
}
// by definition of communications we have 4 integers for coordinates 7 bytes each
const int BytesInNumber = 7; // why 7?
// section of motors' coordinates defines
enum Coordinates { X0, Y0, X1, Y1 };
const int NumberOfCoordinates = 4;
long currentCoordinates[NumberOfCoordinates] = {};

// plane 0 is controlled by 2 motors (X, Y)
#define PLANE_0 0
// plane 1 is the same (two motors more)
#define PLANE_1 1

enum State { IDLE, MOVING, HOMING }; //HOMING is still to be used
State CncState = IDLE;

// motors instantiation
// motor shield object with the default I2C address
Adafruit_MotorShield AdaShields[] = { 
	Adafruit_MotorShield(0x60),
    Adafruit_MotorShield(0x61)
};

// Connect a stepper motors with 200 steps per revolution (1.8 degree)
Adafruit_StepperMotor* Motors[NumberOfCoordinates] = {
	AdaShields[PLANE_0].getStepper(200, 1), // to motor port #1 (M1 and M2) on first shield, X0
	AdaShields[PLANE_0].getStepper(200, 2), // to motor port #2 (M3 and M4) on first shield, Y0
	AdaShields[PLANE_1].getStepper(200, 1), // and to motor port #1 (M3 and M4) on second shield, X1
	AdaShields[PLANE_1].getStepper(200, 2) // and to motor port #1 (M3 and M4) on second shield, Y1
};
// this thing wraps around all motors and moving actions
void forwardstepX0()
{
	Motors[X0]->onestep(FORWARD, SINGLE);
}
void backwardstepX0()
{
	Motors[X0]->onestep(BACKWARD, SINGLE);
}
void forwardstepY0()
{
	Motors[Y0]->onestep(FORWARD, SINGLE);
}
void backwardstepY0()
{
	Motors[Y0]->onestep(BACKWARD, SINGLE);
}
void forwardstepX1()
{
	Motors[X1]->onestep(FORWARD, SINGLE);
}
void backwardstepX1()
{
	Motors[X1]->onestep(BACKWARD, SINGLE);
}
void forwardstepY1()
{
	Motors[Y1]->onestep(FORWARD, SINGLE);
}
void backwardstepY1()
{
	Motors[Y1]->onestep(BACKWARD, SINGLE);
}
AccelStepper stepper[] = {
	AccelStepper(forwardstepX0, backwardstepX0),
	AccelStepper(forwardstepY0, backwardstepY0),
	AccelStepper(forwardstepX1, backwardstepX1),
	AccelStepper(forwardstepY1, backwardstepY1)
};

void MoveEveryThing()
{
	dbgPrint("actual movement by steps");
	boolean needMoreMovement = false;
	long delta[NumberOfCoordinates] = {};
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		delta[i] = stepper[i].distanceToGo(); // calculate for each motor
		if (delta[i]) { // if he still needs to step, do it
			dbgPrint("step of");
			dbgPrint(i);
			stepper[i].run();
			needMoreMovement = needMoreMovement || true;
		}
	}
	// we didn't make a single step we must have finished
	if (!needMoreMovement) {
		CncState = IDLE;
		Serial.println("Success"); // TODO: write status line here
	}
};
void setup()
{
	Serial.begin(9600);           // set up Serial library at 9600 bps
	// setup state
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		currentCoordinates[i] = 0;
	}
	dbgPrint("init first shield");
	AdaShields[PLANE_0].begin();
	dbgPrint("init second shield");
	AdaShields[PLANE_1].begin();
	dbgPrint("Ready for work");
}

// serial port buffer
const int BUFFSIZE = 256;
char msgbuf[BUFFSIZE];
int messageLengthReceived = 0;

// moves all content in buffer one position forward
void shiftBuffer(char *buffer)
{
	dbgPrint("shift buffer");
	for (int i = 1; i < BUFFSIZE; i++) {
		buffer[i - 1] = buffer[i];
	}
	messageLengthReceived = BUFFSIZE - 1;
}
// checks port for incoming data
int checkSerial()
{
	while (Serial.available() > 0) {
		int ch = Serial.read();
		if (ch < 0) {
			Serial.println("!Error: read from serial port failed");
			return 0;
		}
        if (messageLengthReceived == BUFFSIZE) {
			// shift buffer - it is overflowed
			shiftBuffer(msgbuf);
		}
    	msgbuf[messageLengthReceived++] = char(ch);
        if (char(ch) == '~') {
			messageLengthReceived = 0;
			return 1; // message was received
		}
  }
  return 0; // no message received yet
}
long parseUint(char *str, int n)
{
	assert(n == BytesInNumber);
	long res = 0;
	for (int j = 0; j < n; ++j) {
		res = res * 10 + int(str[j] - '0');
	}
	return res;
}
// 1 if success, 0 if error
void parseCoordinates(unsigned long *coordinates)
{
	dbgPrint("parse coordinates");
	// first symbol should be 'G' - if it's not we shouldn't be here at first place
	assert(msgbuf[0] == 'G');
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		coordinates[i] = parseUint(msgbuf + BytesInNumber * i, BytesInNumber);
	}
}
enum CommandType { CT_NOT_VALID, CT_MOVE, CT_HOME, CT_TEMPERATURE, CT_RELEASING };
class Command
{
	CommandType _type;
	// command data. fields are filled according to type. if type is MOVE than temperature is junk, for example
	// data for moving
	unsigned long _coordinates[NumberOfCoordinates];
	// data for homing
	//.... TODO ...
	// data for temperature
	// .... TODO ....
	// data for motor releasing
	// .... TODO .....
public:
	//FIXME maybe we could use inheritance and new/delete. I don't know
	Command() : _type(CT_NOT_VALID) {}
	CommandType type() { return _type; }
	static Command getCommand(char *buffer) {
		Command ret;
		switch (buffer[0]) {
		case 'G': // read incoming coordinates
			ret._type = CT_MOVE;
			parseCoordinates(ret._coordinates);
			break;
		case 'H':
			//TODO go home to 0,0
			ret._type = CT_HOME;
			break;
		case 'T':
			ret._type = CT_TEMPERATURE;
			//TODO set temperature
			break;
		case 'R':
			ret._type = CT_RELEASING;
			// TODO release motors
			break;
		default:
			Serial.print("!Error: unrecognized command ");
			Serial.println(msgbuf);
		}
		return ret;
	}
	void setEnvVariables() {
		switch (this->_type) {
		case CT_MOVE:
			// move motors
			CncState = MOVING;
			// change one coordinate by one exact motor
			dbgPrint("moving of motors");
			for (int i = 0; i < NumberOfCoordinates; ++i) {
				stepper[i].move(this->_coordinates[i] - currentCoordinates[i]);
				//TODO set speeds
				currentCoordinates[i] = this->_coordinates[i];
			}
			break;
		//TODO other commands
		default:
			return;
		}
	}
};

void loop()
{
	if (checkSerial() == 0) {
		if (CncState == IDLE) {
			static int idleLoopCounter = 0;
			if (++idleLoopCounter > 500) {
				idleLoopCounter = 0;
				Serial.println("!idle");
				delay(1);
			}
		}
	} else {
		// parse command
		Command cmd = Command::getCommand(msgbuf);
		if (cmd.type() == CT_NOT_VALID) {
			Serial.println("!Error: check your command type");
		} else {
			dbgPrint("set environment for command");
			cmd.setEnvVariables();
		}
	}
	if (CncState == MOVING)
		return MoveEveryThing();
 }


