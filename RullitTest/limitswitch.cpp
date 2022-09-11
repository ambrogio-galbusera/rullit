#include <Arduino.h>
#include "limitswitch.h"

LimitSwitch::LimitSwitch(int pin)
  : _pin(pin)
{
  pinMode(_pin, INPUT_PULLUP);
}
        
void LimitSwitch::init()
{
  _status = digitalRead(_pin);
  _changeMs = 0;
}

bool LimitSwitch::update()
{
  int status = digitalRead(_pin);
  if (status != _status)
  {
    if (_changeMs = 0)
    {
      _changeMs = millis();    
    }

    unsigned long delta = millis() - _changeMs;
    if (delta > DEBOUNCE_MS)
      _status = status;
  }
  else
  {
    _changeMs = 0;    
  }
}

bool LimitSwitch::closed()
{
  return (_status == 0);
}

bool LimitSwitch::open()
{
  return (_status == 1);
}
