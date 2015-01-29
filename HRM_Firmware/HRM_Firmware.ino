#include <assert.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <AccelStepper.h>
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
long currentCoordinates[NumberOfCoordinates] = { 
};
const int MotorsCount = 1; //TODO will be NumberOfCoordinates

// motors instantiation
// motor shield object with the default I2C address
Adafruit_MotorShield AdaShields[] = { 
  Adafruit_MotorShield() //,
    //Adafruit_MotorShield() // TODO different I2C address for second shield (stacked one)???
  };

  // Connect a stepper motors with 200 steps per revolution (1.8 degree)
Adafruit_StepperMotor* Motors[NumberOfCoordinates] = {
  AdaShields[PLANE_0].getStepper(200, 2) // to motor port #1 (M1 and M2)
    //AdaShields[PLANE_0].getStepper(200, 2) // and to motor port #2 (M3 and M4)
    //TODO motors for second plane
  };

  void forwardstep1() {
    Motors[0]->onestep(FORWARD, SINGLE);
  }
void backwardstep1() {
  Motors[0]->onestep(BACKWARD, SINGLE);
}
AccelStepper stepperX(forwardstep1, backwardstep1);
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

const int BUFFSIZE = 256;
char msgbuf[BUFFSIZE];
int messageLengthReceived = 0;

int checkSerial() {
  while (Serial.available() > 0) {
    char ch = Serial.read();
    
    if (messageLengthReceived == BUFFSIZE) {
      // shift buffer - it is overfilled
      for (int i=1; i<BUFFSIZE; i++) {
        msgbuf[i-1] = msgbuf[i];
      }
      messageLengthReceived--;
    }
    
    msgbuf[messageLengthReceived++] = ch;
    
    if (ch=='~') {
      messageLengthReceived = 0;
      return 1; // message was received
    }
  }
  
  return 0; // no message received yet
}

long parseUint(char * str, int n) {
  long res = 0;
  for (int j = 0; j < n; ++j) {
    res = res * 10 + (str[j] - '0');
  }
}

// 1 if success, 0 if error
int parseCoordinates(unsigned long *coordinates) {
  // first symbol should be 'G'
  if (msgbuf[0] != 'G') {
    return 0; // error
  }
  
  char * msgbuf_it = msgbuf + 1;
  for (int i = 0; i < NumberOfCoordinates; ++i) {
    coordinates[i] = parseUint(msgbuf_it, BytesInNumber);
    msgbuf_it += BytesInNumber;
  }
  return 1;
}

/*void testInput() {
  unsigned long coordinates[NumberOfCoordinates] = { 
  };
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
}*/

// function for movement from point A to point B in desired direction 
void move(Adafruit_StepperMotor *motor, unsigned long prev,
unsigned long next) {
  //TODO
}

int isIdle = 1; // 1 if cnc is idle, 0 if it's moving
unsigned long newCoordinates[NumberOfCoordinates];
int idleLoopCounter = 0;

void loop() {
  if (checkSerial()) {
    // parse command
    // TODO: add workaround if first symbol is not in the list, but message was successfully received
    switch (msgbuf[0]) {
    case 'G':
      // read incoming coordinates
      if (parseCoordinates(newCoordinates)) {
        isIdle = 0; // switch to "move" state
      } else {
        Serial.println("!Wrong coordinates in G-command");
      }
      break;
    case 'H':
      //TODO go home to 0,0
      break;
    case 'T':
      //TODO set temperature
      break;
    case 'R':
      // TODO release motors
      break;
    default:
      Serial.print("!Error: unrecognized command ");
      Serial.println(msgbuf);
      return;
    }
  } else {
    if (isIdle) {
      if (++idleLoopCounter > 500) {
        idleLoopCounter = 0;
        Serial.println("!idle");
      }
    }
    delay(1);
  }
  
  if (!isIdle) {
    // move motors
    for (int i = 0; i < MotorsCount; ++i) {
      // TODO: PAST STEPPER CODE THERE! (goto newCoordinates[])
      // change one coordinate by one exact motor
      stepperX.move(newCoordinates[i] - currentCoordinates[i]);
      while (stepperX.distanceToGo() > 0) {
        stepperX.run();
      }
      currentCoordinates[i] = newCoordinates[i];
    }
    
    // if move is finished (TODO: add code for detecting that as well):
    Serial.println("Success"); // TODO: write status line here
    isIdle = 1;
  }
}


