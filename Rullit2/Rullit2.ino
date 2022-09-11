#include "display.h"
#include "bluesmirf.h"
#include "limitswitch.h"
#include "dispenser.h"

#define STARTUP_DELAY_MS      5000
#define MOTOR_SPEED_RPM       30.0
#define PULLEY_RADIUS_MM      16.0
#define SECONDS_PER_MINUTE    60.0
#define THREAD_SPEED_MM_S     ((MOTOR_SPEED_RPM * PULLEY_RADIUS_MM * 2 * PI) / SECONDS_PER_MINUTE)
#define CYCLE_TIME_MS         100.0
#define CYCLE_TIME_S          (CYCLE_TIME_MS / 1000.0)
#define SAFETY_MARGIN         0
#define STEP_MM               (THREAD_SPEED_MM_S * CYCLE_TIME_S * (1.0 - SAFETY_MARGIN))
#define PATH_STEP_MM          200.0
#define PATH_STEP_MS          ((PATH_STEP_MM / THREAD_SPEED_MM_S) * 1000.0)
//#define WIDTH_MM              11000.0
#define WIDTH_MM              1500.0
#define PERC_LOW_LIMIT        50
#define WIND_PERC             90
#define WIND_DIR              0
#define UNWIND_DIR            1
#define SOFTSTART_STEPS_DELAY 500

//#define TEST

const float SoftStartSteps[] = {10, 20, 50};

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

typedef enum {
  Status_StartUp,
  Status_Moving,
  Status_Completed  
} APP_STATUS;

RullitDisplay display;
BlueSmirf blueSmirf;
LimitSwitch leftLS(LimitLeftPin);
LimitSwitch rightLS(LimitRightPin);
LimitSwitch frontLS(LimitFrontPin);
LimitSwitch rearLS(LimitRearPin);
Dispenser dispenser(ValvePin);

APP_STATUS status = Status_StartUp;
APP_STATUS prevStatus = Status_StartUp;
unsigned long startUpMs;
long aLength;
long bLength;
int estimatedX;
int estimatedY;
unsigned long startMoveMs;
long startMoveLength;
int currentMotor;
int currentPerc;
int leftPerc;
int rightPerc;

void setup() {
  Serial.begin(115200);
  Serial.println("RULLIT");
  
  // put your setup code here, to run once:
  initMotors();
  initPins();
  initLimitSwitches();
  resetStatus();
  rollerOff();

  display.init();
  display.update();

  blueSmirf.init();
  dispenser.init();

  startUpMs = millis();
}

void loop() {
  unsigned long currMs = millis();
  unsigned long deltaMs;
  bool simulateFront = false;
  
  updateLimitSwitches();
  
  // always check for limit switches and stop motors immediately
  if (leftSwitch() || rightSwitch() || frontSwitch() || rearSwitch())
  {
    stopMotor(LeftMotor);
    stopMotor(RightMotor);
  }

  if ((rightLS.changed() && rightSwitch()) || (rearLS.changed() && rearSwitch()))
  {
    unsigned long delta = abs(rightLS.openMillis() - rearLS.openMillis());

    Serial.print("[");
    Serial.print(status);
    Serial.print("] L");
    Serial.print(leftSwitch()? 1 : 0);
    Serial.print(" F");
    Serial.print(frontSwitch()? 1 : 0);
    Serial.print(" A");
    Serial.print(rearSwitch()? 1 : 0);
    Serial.print(" R");
    Serial.print(rightSwitch()? 1 : 0);
    Serial.print(" AT:");
    Serial.print(rearLS.openMillis());
    Serial.print(" RT:");
    Serial.print(rightLS.openMillis());
    Serial.print(" Dt:");
    Serial.print(delta);
    Serial.println();

    if ((rightLS.openMillis() != 0) && (rearLS.openMillis() != 0) && (delta < 15000))
    {
      rollerOff();
      status = Status_Completed;    
    }
  }

  if (Serial.available())
  {
    int ch = Serial.read();
    Serial.print("Command: ");
    Serial.print(ch);
    Serial.print("; ");
    Serial.print(PATH_STEP_MS);
    Serial.println();
    
    if (ch == 's' || ch == 'S')
    {
      stopMotor(LeftMotor);
      stopMotor(RightMotor);
      status = Status_StartUp;
    }
    else if (ch == 'r' || ch == 'R')
    {
      status = Status_Moving;
      resetStatus();

      stopMotor(LeftMotor);
      stopMotor(RightMotor);
      moveMotor(RightMotor, UNWIND_DIR, WIND_PERC);
    }
    else if (ch == 'u')
    {
      stepMotor(RightMotor, UNWIND_DIR, WIND_PERC);
    }
    else if (ch == 'U')
    {
      stepMotor(RightMotor, WIND_DIR, WIND_PERC);
    }
    else if (ch == 'y')
    {
      stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);
    }
    else if (ch == 'Y')
    {
      stepMotor(LeftMotor, WIND_DIR, WIND_PERC);
    }
    else if (ch == 'i' || ch == 'I')
    { 
      stopMotor(LeftMotor);
      stopMotor(RightMotor);

      moveMotor(RightMotor, WIND_DIR, WIND_PERC);
    }
    else if (ch == 'l' || ch == 'L')
    {
      stopMotor(LeftMotor);
      stopMotor(RightMotor);

      moveMotor(LeftMotor, UNWIND_DIR, WIND_PERC);
    }    
    else if (ch == 't')
    {
      simulateFront = true;
    }
  }
  
  if (!blueSmirf.manualMode())
  {
    switch (blueSmirf.command())
    {
      case Command_Start:
        if (status != Status_Moving)
        {
          rollerOn();
          dispenser.start();        
          moveMotor(RightMotor, UNWIND_DIR, WIND_PERC);
          
          status = Status_Moving;
        }
        break;
        
      case Command_Stop:
        if (status == Status_Moving)
        {
          stopMotor(LeftMotor);
          stopMotor(RightMotor);
          rollerOff();
          dispenser.stop();
  
          status = Status_Completed;
        }
        break;
    }

    if (leftLS.changed() || frontLS.changed() || rearLS.changed() || rightLS.changed())
    {
      Serial.print("[");
      Serial.print(status);
      Serial.print("] L");
      Serial.print(leftSwitch()? 1 : 0);
      Serial.print(" F");
      Serial.print(frontSwitch()? 1 : 0);
      Serial.print(" A");
      Serial.print(rearSwitch()? 1 : 0);
      Serial.print(" R");
      Serial.print(rightSwitch()? 1 : 0);
      Serial.println();
    }
        
    if (status == Status_Moving)
    {
      if (leftSwitch())
      {
        handleLeftLimit();
      }
      else if (rearSwitch())
      {
        handleBottomLimit();
      }
      else if (frontSwitch() || simulateFront)
      {
        simulateFront = false;
        handleTopLimit();
      }
      else if (rightSwitch())
      {
        handleRightLimit();
      }
    }
  }
  else
  {
    switch (blueSmirf.command())
    {
      case Command_Stop:
        stopMotor(LeftMotor);
        stopMotor(RightMotor);
        break;
        
      case Command_LeftUnwind:
        moveMotor(LeftMotor, UNWIND_DIR, WIND_PERC);
        break;

      case Command_LeftWind:
        moveMotor(LeftMotor, WIND_DIR, WIND_PERC);
        break;

      case Command_RightUnwind:
        moveMotor(RightMotor, UNWIND_DIR, WIND_PERC);
        break;

      case Command_RightWind:
        moveMotor(RightMotor, WIND_DIR, WIND_PERC);
        break;
    }
  }

  updateRopeLengths();

  if (blueSmirf.connected())
  {
    blueSmirf.rollerOn()? rollerOn() : rollerOff();
    blueSmirf.valveOn()? valveOn() : valveOff();
  }

  dispenser.update();

  display.setConnected(blueSmirf.connected());
  display.setMode(blueSmirf.manualMode()? Mode_Manual : Mode_Auto);
  display.setCommand((int)blueSmirf.command());
  display.setSwitch(Switch_Left, leftSwitch());
  display.setSwitch(Switch_Front, frontSwitch());
  display.setSwitch(Switch_Rear, rearSwitch());
  display.setSwitch(Switch_Right, rightSwitch());
  display.setStatus(status);
  display.setPos(estimatedX, estimatedY);
  display.setLengths(aLength, bLength);
  display.setPWM(leftPerc, rightPerc);
  display.setValve(dispenser.valve());
  display.update();

  blueSmirf.setAppStatus((int)status);
  blueSmirf.setSwitches(leftSwitch(), frontSwitch(), rearSwitch(), rightSwitch());
  blueSmirf.setPos(estimatedX, estimatedY);
  blueSmirf.setLengths(aLength, bLength);
  blueSmirf.setPWM(leftPerc, rightPerc);
  blueSmirf.setValve(dispenser.valve());
  blueSmirf.update();
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
  leftLS.resetChanged();  
  rightLS.resetChanged();  
  frontLS.resetChanged();  
  rearLS.resetChanged();  

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
  digitalWrite(RollerPin, LOW);
  blueSmirf.setRoller(true);
  display.setRoller(true);
}

void rollerOff()
{
  digitalWrite(RollerPin, HIGH);
  blueSmirf.setRoller(false);
  display.setRoller(false);
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
#ifndef TEST
  return (leftLS.open());  
#else
  return false;
#endif
}

bool rightSwitch()
{
#ifndef TEST
  return (rightLS.open());  
#else
  return false;
#endif
}

bool frontSwitch()
{
#ifndef TEST
  return (frontLS.open());  
#else
  return false;
#endif
}

bool rearSwitch()
{
#ifndef TEST
  return (rearLS.open());  
#else
  return false;
#endif
}

void stepMotor(int motor, int dir, int perc)
{
  Serial.print("stepMotor ");
  Serial.print((motor == LeftMotor)? "LEFT " : "RIGHT ");
  Serial.print((dir == WIND_DIR)? "WIND " : "UNWIND ");
  Serial.print(perc);
  Serial.print("%");
  Serial.println();

  unsigned long end = millis() + PATH_STEP_MS;
  moveMotor(motor, dir, perc);
  while (millis() < end)
  {
    updateRopeLengths();

    display.setSwitch(Switch_Left, leftSwitch());
    display.setSwitch(Switch_Front, frontSwitch());
    display.setSwitch(Switch_Rear, rearSwitch());
    display.setSwitch(Switch_Right, rightSwitch());
    display.setStatus(status);
    display.setPos(estimatedX, estimatedY);
    display.setLengths(aLength, bLength);
    display.setPWM(leftPerc, rightPerc);
    display.setValve(dispenser.valve());
    display.update();
  
    blueSmirf.setAppStatus((int)status);
    blueSmirf.setSwitches(leftSwitch(), frontSwitch(), rearSwitch(), rightSwitch());
    blueSmirf.setPos(estimatedX, estimatedY);
    blueSmirf.setLengths(aLength, bLength);
    blueSmirf.setPWM(leftPerc, rightPerc);
    blueSmirf.setValve(dispenser.valve());
    blueSmirf.update();

    delay(1);
  }
  stopMotor(motor);

  Serial.println("stepMotor completed");
}

void handleLeftLimit()
{
  Serial.println(">>> LEFT LIMIT");
  stepMotor(RightMotor, WIND_DIR, WIND_PERC);
  stepMotor(LeftMotor, UNWIND_DIR , WIND_PERC);

  Serial.println("moveMotor RIGHT WIND");
  moveMotor(RightMotor, WIND_DIR, WIND_PERC);
}

void handleBottomLimit()
{
  Serial.println(">>> BOTTOM LIMIT");
  stepMotor(LeftMotor, WIND_DIR, WIND_PERC);
  stepMotor(RightMotor, WIND_DIR, WIND_PERC);
  stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);
  stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);

  Serial.println("moveMotor RIGHT WIND");
  moveMotor(RightMotor, WIND_DIR, WIND_PERC);
}

void handleTopLimit()
{
  Serial.println(">>> TOP LIMIT");
  stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);
  stepMotor(RightMotor, WIND_DIR, WIND_PERC);
  stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);

  Serial.println("moveMotor RIGHT UNWIND");
  moveMotor(RightMotor, UNWIND_DIR, WIND_PERC);
}

void handleRightLimit()
{
  Serial.println(">>> RIGHT LIMIT");
  stepMotor(RightMotor, UNWIND_DIR, WIND_PERC);
  stepMotor(LeftMotor, UNWIND_DIR, WIND_PERC);

  Serial.println("moveMotor RIGHT UNWIND");
  moveMotor(RightMotor, UNWIND_DIR, WIND_PERC);
}

void resetStatus()
{
  aLength = 0;
  bLength = WIDTH_MM;

  updateEstimatedCoords();
}

void updateRopeLengths()
{
  if (currentMotor == -1)
    return;

  unsigned long delta = millis() - startMoveMs;
  float length = (THREAD_SPEED_MM_S * (float)delta * ((float)currentPerc / 100.0)) / 1000.0;

  if (currentMotor == LeftMotor)
    aLength = startMoveLength + length;
  else if (currentMotor == RightMotor)
    bLength = startMoveLength - length;

  if (aLength < 0)
    aLength = 0;

  if (bLength < 0)
    bLength = 0;

  updateEstimatedCoords();
}

void updateEstimatedCoords()
{
    float a = aLength;
  float b = bLength;
  float x = ((a * a) - (b * b) + (WIDTH_MM * WIDTH_MM)) / (2 * WIDTH_MM);
  estimatedX = (int)x;

  float y = sqrt((a * a) - (x * x));    
  estimatedY = (int)y;
}

void stopMotor(int motorNumber)
{
  if (currentPerc != 0)
  {
    Serial.print("STOP ");
    Serial.print((motorNumber == LeftMotor)? "LEFT " : "RIGHT ");
    Serial.println();
  }
  
  updateRopeLengths();
  moveMotor(motorNumber, 0);
}

void moveMotor(int motorNumber, int dir, float perc)
{
  if (currentMotor != motorNumber)
  {
    stopMotor(LeftMotor);
    stopMotor(RightMotor);
  }
  
  float sign = +1.0;
  
  if (motorNumber == LeftMotor)  {
    if (dir == WIND_DIR)
      sign = -1.0;
  }
  else {
    if (dir == UNWIND_DIR)
      sign = -1.0;
  }

  if (currentPerc != 0)
  {
    if (currentPerc == (sign * perc))
      return;
    
    stopMotor(currentMotor);
  }

  for (int i=0; i<sizeof(SoftStartSteps)/sizeof(float); i++)
  {
    moveMotor(motorNumber, sign * SoftStartSteps[i]);
    delay(SOFTSTART_STEPS_DELAY);
  }

  moveMotor(motorNumber, sign * perc);
}

void moveMotor(int motorNumber, float perc)
{
  currentMotor = (perc != 0)? motorNumber : -1;
  currentPerc = perc;
  startMoveLength = (motorNumber == LeftMotor)? aLength : bLength;
  startMoveMs = millis();

  leftPerc = (motorNumber == LeftMotor)? perc : 0;
  rightPerc = (motorNumber == RightMotor)? perc : 0;

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
