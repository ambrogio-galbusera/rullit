#include <Arduino.h>
#include "dispenser.h"

Dispenser::Dispenser(int pin)
  : _pin(pin)
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin ,LOW);
}
        
void Dispenser::init()
{
  _changeMs = 0;
  _status = LOW;
  _overridden = false;
}

bool Dispenser::update()
{
  if (_overridden)
    return;
  
  unsigned long now = millis();
  unsigned long delta = now - _changeMs;

  if (_status == LOW)
  {
    if (delta > VALVE_CLOSED_MS)
    {
      _status = HIGH;
      _changeMs = millis();
      digitalWrite(_pin ,_status);
    }
  }
  else
  {
    if (delta > VALVE_OPEN_MS)
    {
      _status = LOW;
      _changeMs = millis();
      digitalWrite(_pin ,_status);
    }
  }
}

bool Dispenser::valve()
{
  return (_status == HIGH);
}

void Dispenser::setValve(bool on)
{
  _overridden = true;
  _status = on? HIGH : LOW;
  digitalWrite(_pin ,_status);
}
