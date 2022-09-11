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
#define RIGHT_STEP_MM         200
//#define WIDTH_MM              11000.0
#define WIDTH_MM              1500.0
#define PERC_LOW_LIMIT        50

//#define TEST

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
  Status_MovingDown,
  Status_MovingRight,
  Status_MovingUp,
  Status_FinalHoming,
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
bool cycleRunning = false;
unsigned long cycleStartMs;
unsigned long leftOnMs = 0;
unsigned long rightOnMs = 0;
float evaluatedX = 0;
float evaluatedY = 0;
float rightEvaluatedX;
float aLength = 0;
float bLength = WIDTH_MM;
float leftPerc = 0;
float rightPerc = 0;
int motorToMove = LeftMotor;

void setup() {
  Serial.begin(115200);
  Serial.println("RULLIT");
  
  // put your setup code here, to run once:
  initMotors();
  initPins();
  initLimitSwitches();

  display.init();
  display.update();

  blueSmirf.init();
  dispenser.init();

  startUpMs = millis();
}

void loop() {
  unsigned long currMs = millis();
  unsigned long deltaMs;

  updateLimitSwitches();
  
  // always check for limit switches and stop motors immediately
  if (leftSwitch() || rightSwitch() || frontSwitch() || rearSwitch())
  {
    stopMotor(LeftMotor);
    stopMotor(RightMotor);
  }

  if (rightSwitch())
  {
    rollerOff();
    status = Status_FinalHoming;    
  }

  if (cycleRunning)
  {
    deltaMs = currMs - cycleStartMs;
    if (deltaMs >= leftOnMs)
      stopMotor(LeftMotor);

    if (deltaMs >= rightOnMs)
      stopMotor(RightMotor);

    if ((deltaMs >= leftOnMs) && (deltaMs >= rightOnMs) && (deltaMs >= CYCLE_TIME_MS))
      cycleRunning = false;
  }

  if (!cycleRunning)
  {
    if (Serial.available())
    {
      int ch = Serial.read();
      if (ch == 's' || ch == 'S')
      {
        stopMotor(LeftMotor);
        stopMotor(RightMotor);
        status = Status_StartUp;
      }
      if (ch == 'r' || ch == 'R')
      {
        if (status == Status_StartUp)
          status = Status_MovingDown;
      }
    }
    
    if (!blueSmirf.manualMode())
    {
      switch (blueSmirf.command())
      {
        case Command_Start:
          evaluatedX = 0;
          evaluatedY = 0;
          aLength = 0;
          bLength = WIDTH_MM;

          rollerOn();
          
          status = Status_MovingDown;
          break;
          
        case Command_Stop:
          stopMotor(LeftMotor);
          stopMotor(RightMotor);
          rollerOff();

          status = Status_Completed;
          break;
      }
      
      switch (status)
      {
        case Status_StartUp:
#ifdef TEST
          status = Status_MovingDown;
#endif
          break;
    
        case Status_MovingDown:
#ifndef TEST
          if (rearSwitch())
#else
          if (evaluatedY > 4000)
#endif
          {
            prevStatus = status;
            status = Status_MovingRight;
  
            rightEvaluatedX = evaluatedX;
            evaluatedX += STEP_MM;
            moveTo(evaluatedX, evaluatedY);     
          }
          else
          {
            evaluatedY += STEP_MM;
            moveTo(evaluatedX, evaluatedY);     
          }
          break;
    
        case Status_MovingRight:
          if (evaluatedX - rightEvaluatedX < RIGHT_STEP_MM)
          {
            evaluatedX += STEP_MM;
            moveTo(evaluatedX, evaluatedY);     
          }
          else
            status = (prevStatus == Status_MovingDown)? Status_MovingUp : Status_MovingDown;      
          break;
          
        case Status_MovingUp:
#ifndef TEST
          if (frontSwitch())
#else
          if (evaluatedY <= 0)
#endif
          {
            prevStatus = status;
            status = Status_MovingRight;
            
            rightEvaluatedX = evaluatedX;
  
            evaluatedY = 0;
            evaluatedX += STEP_MM;
            moveTo(evaluatedX, evaluatedY);     
          }
          else
          {
            evaluatedY -= STEP_MM;
            moveTo(evaluatedX, evaluatedY);     
          }
          break;
    
        case Status_FinalHoming:
          evaluatedX -= STEP_MM;
          if (evaluatedX < 0)
            evaluatedX = 0;
            
          evaluatedY -= STEP_MM;
          if (evaluatedY < 0)
            evaluatedY = 0;
            
          moveTo(evaluatedX, evaluatedY);     
          break;
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
          
        case Command_Left:
          evaluatedX -= STEP_MM;
          if (evaluatedX < 0)
            evaluatedX = 0;
            
          moveTo(evaluatedX, evaluatedY);     
          break;

        case Command_Up:
          evaluatedY -= STEP_MM;
          if (evaluatedY < 0)
            evaluatedY = 0;
            
          moveTo(evaluatedX, evaluatedY);     
          break;

        case Command_Down:
          evaluatedY += STEP_MM;            
          moveTo(evaluatedX, evaluatedY);     
          break;

        case Command_Right:
          evaluatedX += STEP_MM;            
          moveTo(evaluatedX, evaluatedY);     
          break;
      }
    }

    if (blueSmirf.connected())
    {
      blueSmirf.rollerOn()? rollerOn() : rollerOff();
      blueSmirf.valveOn()? valveOn() : valveOff();
    }
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
  display.setPos(evaluatedX, evaluatedY);
  display.setLengths(aLength, bLength);
  display.setPWM(leftPerc, rightPerc);
  display.setValve(dispenser.valve());
  display.update();

  blueSmirf.setAppStatus((int)status);
  blueSmirf.setSwitches(leftSwitch(), frontSwitch(), rearSwitch(), rightSwitch());
  blueSmirf.setPos(evaluatedX, evaluatedY);
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
  blueSmirf.setRoller(true);
  display.setRoller(true);
}

void rollerOff()
{
  digitalWrite(RollerPin, LOW);
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
  //return (leftLS.open());  
  return false;
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

void stopMotor(int motorNumber)
{
  moveMotor(motorNumber, 0);
  cycleRunning = false;
}

void moveTo(float x, float y)
{
  float a = sqrt(x*x + y*y);
  float b = sqrt((WIDTH_MM-x)*(WIDTH_MM-x) + y*y);

  float aDelta = a - aLength;
  float bDelta = b - bLength;

  float leftPerc = (aDelta * 100.0 / CYCLE_TIME_S) / THREAD_SPEED_MM_S;
  float rightPerc = (bDelta * 100.0 / CYCLE_TIME_S) / THREAD_SPEED_MM_S;

  if (leftPerc > 100.0)
  {
    leftPerc = 100.0;
    aDelta = CYCLE_TIME_S * THREAD_SPEED_MM_S;
    a = aLength + aDelta;
  }
   
  if (rightPerc > 100.0)
  {
    rightPerc = 100.0;
    bDelta = CYCLE_TIME_S * THREAD_SPEED_MM_S;
    b = bLength + bDelta;
  }
  
  leftOnMs = CYCLE_TIME_MS;
  if (fabs(leftPerc) < PERC_LOW_LIMIT)
  {
    leftPerc = 0;
  }

  rightOnMs = CYCLE_TIME_MS;
  if (fabs(rightPerc) < PERC_LOW_LIMIT)
  {
    rightPerc = 0;
  }

  Serial.print("Moving [");
  Serial.print(status);
  Serial.print("/ ");
  Serial.print(STEP_MM);
  Serial.print("] ");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(" --> ");
  Serial.print(a);
  Serial.print(", ");
  Serial.print(b);
  Serial.print(" --> ");
  Serial.print(aDelta);
  Serial.print(" / ");
  Serial.print(bDelta);
  Serial.print(" --> ");
  Serial.print(leftPerc);
  Serial.print(" / ");
  Serial.print(rightPerc);
  Serial.print(" [L");
  Serial.print(leftSwitch()? 1 : 0);
  Serial.print(" R");
  Serial.print(rightSwitch()? 1 : 0);
  Serial.print(" F");
  Serial.print(frontSwitch()? 1 : 0);
  Serial.print(" A");
  Serial.print(rearSwitch()? 1 : 0);
  Serial.print("]");
  Serial.println();

  if ((motorToMove == LeftMotor) && (leftPerc != 0))
  {
    moveMotor(LeftMotor, leftPerc);
    motorToMove = RightMotor;
    rightPerc = 0;
  }
  
  if ((motorToMove == RightMotor) && (rightPerc != 0))
  {
    moveMotor(RightMotor, -rightPerc);
    motorToMove = LeftMotor;
    leftPerc = 0;
  }
  
  if (leftPerc != 0)
    aLength = a;

  if (rightPerc != 0)
    bLength = b;
  
  cycleRunning = true;
  cycleStartMs = millis();
}

void moveMotor(int motorNumber, float perc)
{
  if (motorNumber == LeftMotor)
    leftPerc = perc;
  else
    rightPerc = perc;
    
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
