#include "limitswitch.h"
#include "dispenser.h"

/** limit switches **/
const int LimitLeftPin = 7;
const int LimitRightPin = 6;
const int LimitFrontPin = 4;
const int LimitRearPin = 5;

/** encoder switches **/
//const int EncoderLeftPin = 6;
//const int EncoderRightPin = 7;

/** output pins **/
const int RollerPin = 8;
const int ValvePin = 9;

/**set control port**/
const int E1Pin = 10;
const int M1Pin = 12;
const int E2Pin = 11;
const int M2Pin = 13;

/**inner definition**/
typedef struct {
  byte enPin;
  byte directionPin;
} MotorContrl;

const int LeftMotor = 0;
const int RightMotor = 1;
const int MotorNum = 2;

const MotorContrl MotorPin[] = { {E1Pin, M1Pin}, {E2Pin, M2Pin} } ;

const int Forward = LOW;
const int Backward = HIGH;

LimitSwitch leftLS(LimitLeftPin);
LimitSwitch rightLS(LimitRightPin);
LimitSwitch frontLS(LimitFrontPin);
LimitSwitch rearLS(LimitRearPin);
Dispenser dispenser(ValvePin);

int leftPerc = 0;
int rightPerc = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("RULLIT");
  
  // put your setup code here, to run once:
  initMotors();
  initPins();
  initLimitSwitches();
}

void loop() {
  updateLimitSwitches();
  
  Serial.print("F: ");
  Serial.print(frontSwitch()? "1" : "0");
  Serial.print("; A: ");
  Serial.print(rearSwitch()? "1" : "0");
  Serial.print("; L: ");
  Serial.print(leftSwitch()? "1" : "0");
  Serial.print("; R: ");
  Serial.print(rightSwitch()? "1" : "0");
  Serial.println();

  if (Serial.available())
  {
    int ch = Serial.read();
    switch (ch)
    {
      case 'R':
        rollerOn();
        break;
      case 'r':
        rollerOff();
        break;
      case 'V':
        dispenser.setValve(true);
        break;
      case 'v':
        dispenser.setValve(false);
        break;
      case '1':
        leftPerc = (leftPerc + 10) % 100;
        Serial.print("Moving left motor ");
        Serial.println(leftPerc);
        moveMotor(LeftMotor, leftPerc);
        break;
      case '2':
        rightPerc = 90; //(rightPerc + 10) % 100;
        Serial.print("Moving right motor ");
        Serial.println(rightPerc);
        moveMotor(RightMotor, rightPerc);
        break;
      case '4':
        rightPerc = 0; //(rightPerc + 10) % 100;
        Serial.print("Moving right motor ");
        Serial.println(rightPerc);
        moveMotor(RightMotor, rightPerc);
        break;
    }
  }

  delay(1000);

  dispenser.update();
}

/**functions**/
void initMotors( ) {
  int i;
  for ( i = 0; i < MotorNum; i++ ) {
    digitalWrite(MotorPin[i].enPin, LOW);

    pinMode(MotorPin[i].enPin, OUTPUT);
    pinMode(MotorPin[i].directionPin, OUTPUT);
  }
}

void initLimitSwitches()
{
  leftLS.init();  
  rightLS.init();  
  frontLS.init();  
  rearLS.init();  
}

void updateLimitSwitches()
{
  leftLS.update();  
  rightLS.update();  
  frontLS.update();  
  rearLS.update();  
}

void initPins( ) {
  pinMode(RollerPin, OUTPUT);
  pinMode(ValvePin, OUTPUT);
}

void rollerOn()
{
  digitalWrite(RollerPin, HIGH);
}

void rollerOff()
{
  digitalWrite(RollerPin, LOW);
}

void valveOn()
{
  dispenser.setValve(true);
}

void valveOff()
{
  dispenser.setValve(false);
}

bool leftSwitch()
{
  return (leftLS.closed());  
}

bool rightSwitch()
{
  return (rightLS.closed());  
}

bool frontSwitch()
{
  return (frontLS.closed());  
}

bool rearSwitch()
{
  return (rearLS.closed());  
}

void moveMotor(int motorNumber, float perc)
{
  setMotorDirection(motorNumber, (perc < 0)? 1 : 0);
  setMotorSpeed(motorNumber, (int)fabs(perc));
}

/**  motorNumber: M1, M2
direction:          Forward, Backward **/
void setMotorDirection( int motorNumber, int direction ) {
  digitalWrite( MotorPin[motorNumber].directionPin, direction);
}

/** speed:  0-100   * */
inline void setMotorSpeed( int motorNumber, int speed ) {
  analogWrite(MotorPin[motorNumber].enPin, 255.0 * (speed / 100.0) ); //PWM
}
