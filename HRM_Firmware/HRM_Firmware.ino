#include <assert.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <AccelStepper.h>
#include "utility/Adafruit_PWMServoDriver.h"

#define _DBG_
// by definition of communications we have 4 integers for coordinates 7 bytes each
const int NumberOfCoordinates = 4;
const int BytesInNumber = 7; // why 7?

// plane 0 is controlled by 2 motors (X, Y)
#define PLANE_0 0
// plane 1 is the same (two motors more)
#define PLANE_1 1

enum State { IDLE, MOVING, HOMING }; //HOMING is still to be used
State CncState = IDLE;

// section of motors' coordinates defines
long currentCoordinates[NumberOfCoordinates] = {};
const int MotorsCount = 1; //TODO will be NumberOfCoordinates

// motors instantiation
// motor shield object with the default I2C address
Adafruit_MotorShield AdaShields[] = { 
	Adafruit_MotorShield(0x60),
    Adafruit_MotorShield(0x61)
 };

// Connect a stepper motors with 200 steps per revolution (1.8 degree)
Adafruit_StepperMotor* Motors[NumberOfCoordinates] = {
	//AdaShields[PLANE_0].getStepper(200, 1), // to motor port #1 (M1 and M2) on first shield, X0
	AdaShields[PLANE_0].getStepper(200, 2), // to motor port #2 (M3 and M4) on first shield, Y0
	AdaShields[PLANE_1].getStepper(200, 1) // and to motor port #1 (M3 and M4) on second shield, X1
	//AdaShields[PLANE_1].getStepper(200, 2) // and to motor port #1 (M3 and M4) on second shield, Y1
};
void dbgPrint(String str)
{
#ifdef _DBG_
	Serial.println(str);
#endif
}
void setup()
{
	Serial.begin(9600);           // set up Serial library at 9600 bps
	// setup state
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		currentCoordinates[i] = 0;
	}
	dbgPrint("before first shield");
	AdaShields[PLANE_0].begin();
	dbgPrint("before second shield");
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
	Serial.println("shift buffer");
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
	// first symbol should be 'G' - if it's not we shouldn't be here at first place
	assert(msgbuf[0] == 'G');
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		coordinates[i] = parseUint(msgbuf + BytesInNumber * i, BytesInNumber);
	}
}
// this thing wraps around all motors and moving actions
void forwardstepY0() { Motors[0]->onestep(FORWARD, SINGLE); }
void backwardstepY0() { Motors[0]->onestep(BACKWARD, SINGLE); }
void forwardstepX1() { Motors[1]->onestep(FORWARD, SINGLE); }
void backwardstepX1() { Motors[1]->onestep(BACKWARD, SINGLE); }
AccelStepper stepperY0(forwardstepY0, backwardstepY0);
AccelStepper stepperX1(forwardstepX1, backwardstepX1);

void MoveEveryThing()
{
	int needMoreMoves = 0;
	// TODO stepperX1
	while (stepperY0.distanceToGo() > 0) {
		needMoreMoves++;
		stepperY0.run();
	}
	while (stepperX1.distanceToGo() > 0) {
		needMoreMoves++;
		stepperX1.run();
	}
	// TODO stepperY1
	if (needMoreMoves == 0) {
		CncState = IDLE;
		Serial.println("Success"); // TODO: write status line here
	}
};

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
			//stepperX0
			stepperY0.move(this->_coordinates[0] - currentCoordinates[0]);
			stepperX1.move(this->_coordinates[1] - currentCoordinates[1]);
			//stepperY1
			for (int i = 0; i < NumberOfCoordinates; ++i)
				currentCoordinates[i] = this->_coordinates[i];
			break;
		//TODO others
		default:
			return;
		}
	}
};

void loop()
{
	Serial.println("TEST3");
	delay(1000);
	return;
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
		// TODO: add workaround if first symbol is not in the list, but message was successfully received
		Command cmd = Command::getCommand(msgbuf);
		if (cmd.type() == CT_NOT_VALID) {
			Serial.println("!Error: check your command type");
		} else {
			cmd.setEnvVariables();
		}
	}
	if (CncState == MOVING)
		return MoveEveryThing();
 }


