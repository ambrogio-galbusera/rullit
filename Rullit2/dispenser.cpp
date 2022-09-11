#include <Arduino.h>
#include "dispenser.h"

#define VALVE_CLOSED    HIGH
#define VALVE_OPEN      LOW
#define SENSOR_PIN      A0
#define SENSOR_SAMPLES  10
#define SENSOR_THRESHOLD 500
#define SAMPLETIME_MS   500

Dispenser::Dispenser(int pin)
  : _pin(pin)
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin ,LOW);
}
        
void Dispenser::init()
{
  _changeMs = 0;
  _status = VALVE_CLOSED;
  _overridden = false;
  _running = false;
  _samplesSum = 0;
  _samplesNum = 0;
  _sampleMs = 0;

  digitalWrite(_pin ,_status);
}

void Dispenser::start()
{
  _running = true;
  _samplesSum = 0;
  _samplesNum = 0;
}

void Dispenser::stop()
{
  _running = false;
}

bool Dispenser::update()
{
  if (_overridden)
    return;

  if (!_running)
    return;
  
  unsigned long now = millis();
  unsigned long delta = now - _changeMs;

  if (_status == VALVE_CLOSED)
  {
#ifdef MODE_TIMER
    if (delta > VALVE_CLOSED_MS)
    {
      _status = VALVE_OPEN;
      _changeMs = millis();
      digitalWrite(_pin ,_status);
    }
#else
    if (delta > VALVE_CLOSED_MS)
    {
      delta = now - _sampleMs;
      if (delta > SAMPLETIME_MS)
      {
        _sampleMs = millis();
        
        int val = analogRead(SENSOR_PIN);
        _samplesSum += val;
        _samplesNum ++;
        
        if (_samplesNum > SENSOR_SAMPLES) 
        {
          long avg = _samplesSum / _samplesNum;
          if (avg > SENSOR_THRESHOLD)
          {
            _status = VALVE_OPEN;
            _changeMs = millis();
            digitalWrite(_pin ,_status);
          }
          else
          {
            _samplesNum = 0;
            _samplesSum = 0;
          }
        }
      }
    }
#endif
  }
  else
  {
    if (delta > VALVE_OPEN_MS)
    {
      _status = VALVE_CLOSED;
      _changeMs = millis();
      _samplesSum = 0;
      _samplesNum = 0;

      digitalWrite(_pin ,_status);
    }
  }
}

bool Dispenser::valve()
{
  return (_status == VALVE_OPEN);
}

void Dispenser::setValve(bool on)
{
  _overridden = true;
  _status = on? VALVE_OPEN : VALVE_CLOSED;
  digitalWrite(_pin ,_status);
}
