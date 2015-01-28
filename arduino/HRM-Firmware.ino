#include <assert.h>
//#include <Wire.h>
//#include <Adafruit_MotorShield.h>
//#include "utility/Adafruit_PWMServoDriver.h"

// by definition of communications we have 4 integers for coordinates 7 bytes each
const int NumberOfCoordinates = 4;
const int BytesInNumber = 7; // why 7?

void setup() {
	Serial.begin(9600);           // set up Serial library at 9600 bps
	Serial.println("Serial test!");
}

void halt()
{
	// do nothing
	for (;;) {}
}
int fillOneSeries(unsigned int *coordinates, unsigned int sz)
{
	assert(sz == NumberOfCoordinates);
	// wait for serial input of coordinates: G-byte and NumberOfCoordinates * BytesInNumber and newline 
	if (Serial.available() < NumberOfCoordinates * BytesInNumber + 2) {
		Serial.println("not enough input");
		delay(1000);
		return -1;
	}
	// first symbol should be 'G'
	int tmp = Serial.read();
	if (tmp < 0) {
		Serial.println("Error: can't read from serial port");
		return -1;
	} else if (char(tmp) != 'G') {
		Serial.print("Error: G should be first, not ");
		Serial.println(char(tmp));
		return -1;
	}
	for (int i = 0; i < NumberOfCoordinates; ++i) {
		coordinates[i] = 0;
		// for each number calculate by bytes
		for (int j = 0; j < BytesInNumber; ++j) {
			tmp = Serial.read();
			if (tmp < 0) {
				Serial.print("Error: no data found ");
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
		Serial.println("Error: no tilda at the end");
		return -1;
	}
	return 0;
}

void loop()
{
	unsigned int coordinates[NumberOfCoordinates] = {};
	while (fillOneSeries(coordinates, NumberOfCoordinates) >= 0) {
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
