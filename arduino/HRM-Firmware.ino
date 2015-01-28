#include <assert.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
//#include "utility/Adafruit_PWMServoDriver.h"

// by definition of communications we have 4 integers for coordinates 7 bytes each
const int NumberOfCoordinates = 4;
const int BytesInNumber = 7; // why 7?

// plane 0 is controlled by 2 motors (X, Y)
#define PLANE_0 0
// plane 1 is the same (two motors more)
#define PLANE_1 1
//TODO rename?

// section of motors' coordinates defines
long currentCoordinates[NumberOfCoordinates] = {};
const int MotorsCount = 2; //TODO will be NumberOfCoordinates

// motors instantiation
// motor shield object with the default I2C address
Adafruit_MotorShield AdaShields[] = {
	Adafruit_MotorShield() //,
	//Adafruit_MotorShield() // TODO different I2C address for second shield (stacked one)???
};

// Connect a stepper motors with 200 steps per revolution (1.8 degree)
Adafruit_StepperMotor* Motors[NumberOfCoordinates] = {
	AdaShields[PLANE_0].getStepper(200, 1), // to motor port #1 (M1 and M2)
	AdaShields[PLANE_0].getStepper(200, 2) // and to motor port #2 (M3 and M4)
	//TODO motors for second plane
};

void setup() {
	Serial.begin(9600);           // set up Serial library at 9600 bps
	// setup state
	// TODO we can write a command to setup an initial state
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		currentCoordinates[i] = 0;
	}
	AdaShields[PLANE_0].begin();
	//TODO second one
	Serial.println("Ready for work");
}

void halt()
{
	// do nothing
	for (;;) {}
}
int fillCoordinates(unsigned long *coordinates, unsigned int sz)
{
	assert(sz == NumberOfCoordinates);
	// first symbol should be 'G'
	int tmp = Serial.read();
	if (tmp < 0) {
		Serial.println("!Error: can't read from serial port");
		return -1;
	} else if (char(tmp) != 'G') {
		Serial.print("!Error: G should be first, not ");
		Serial.println(char(tmp));
		return -1;
	}
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		coordinates[i] = 0;
		// for each number calculate by bytes
		for (int j = 0; j < BytesInNumber; ++j) {
			tmp = Serial.read();
			if (tmp < 0) {
				Serial.print("!Error: no data found ");
				Serial.print(i + 1);
				Serial.print(" ");
				Serial.print(j + 1);
				Serial.println("");
				return -1;
			}
			coordinates[i] = coordinates[i] * 10 + int(char(tmp) - '0');
		}
	}
	if (char(Serial.read()) != '~') {
		Serial.println("!Error: no tilda at the end");
		return -1;
	}
	return 0;
}

void testInput()
{
	unsigned long coordinates[NumberOfCoordinates] = {};
	while (fillCoordinates(coordinates, NumberOfCoordinates) >= 0) {
		// test purposes: print out the result
		for (int i = 0; i < NumberOfCoordinates; ++i) {
			Serial.print(" X");
			Serial.print(i + 1);
			Serial.print("= ");
			Serial.print(coordinates[i]);
		}
		Serial.println("");
		// clean up values
		for (int i = 0; i < NumberOfCoordinates; ++i)
			coordinates[i] = 0;
	}
}

// function for movement from point A to point B in desired direction 
void move(Adafruit_StepperMotor *motor, unsigned long prev, unsigned long next)
{
	//TODO
}

void loop()
{
	// wait for serial input of coordinates: G-byte and NumberOfCoordinates * BytesInNumber and tilda
	if (Serial.available() < NumberOfCoordinates * BytesInNumber + 2) {
		Serial.println("!not enough input");
		delay(1000);
		return;
	}
	// check the type of incoming command
	int ctype = Serial.peek();
	// TODO refactor if into switch or (better) call functions by f-pointers
	if (ctype == int('G')) {
		// read the incoming coordinates
		unsigned long newCoordinates[NumberOfCoordinates] = {};
		if (fillCoordinates(newCoordinates, NumberOfCoordinates) < 0)
			return;
		// move each motor
		for (int i = 0; i < MotorsCount; ++i) {
			// change one coordinate by one exact motor
			move(Motors[i], currentCoordinates[i], newCoordinates[i]);
			currentCoordinates[i] = newCoordinates[i];
		}
	}
	else if (ctype == int('H')) {
		//TODO go home to 0,0
		// ....
	} else if (ctype == int('T')) {
		//TODO set temperature
		// .....
	} else if (ctype == int('R')) {
		// TODO release motors
		// .....
	} else {
		Serial.print("!Error: unrecognized command type ");
		Serial.println(char(ctype));
		return;
	}
	Serial.println("Success");
}
