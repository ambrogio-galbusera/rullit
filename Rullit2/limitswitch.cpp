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
  _openMillis = 0;
  
  if (open())
    _openMillis = millis();
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
    {
        _status = status;
        _changed = true;
        if (open())
          _openMillis = millis();
    }
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

unsigned long LimitSwitch::openMillis()
{
  return _openMillis;
}

bool LimitSwitch::changed()
{
  return _changed;
}

void LimitSwitch::resetChanged()
{
  _changed = false;
}
