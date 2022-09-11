#include "bluesmirf.h"
#include <Arduino.h>
#include <SoftwareSerial.h>  

int bluetoothTx = 2;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 3;  // RX-I pin of bluetooth mate, Arduino D3

SoftwareSerial _bluetooth(bluetoothTx, bluetoothRx);

BlueSmirf::BlueSmirf()
{
  _protoStatus = Proto_WaitSTX;
  _manual = false;
  _command = Command_None;
  _appStatus = 0;
  _rollerCommand = false;
  _rollerStatus = false;
  _valveCommand = false; 
  _valveStatus = false; 
  _lastMsgMs = 0;
  _connected = false;
}
        
void BlueSmirf::init()
{
  _bluetooth.begin(115200);  // The Bluetooth Mate defaults to 115200bps
  _bluetooth.print("$$$");  // Print three times individually
  delay(100);  // Short delay, wait for the Mate to send back CMD

  _bluetooth.println("U,9600,N");  // Temporarily Change the baudrate to 9600, no parity
  // 115200 can be too fast at times for NewSoftSerial to relay the data reliably
  _bluetooth.begin(9600);  // Start bluetooth serial at 9600
  _bluetooth.print("$$$");  // Print three times individually
  delay(100);  // Short delay, wait for the Mate to send back CMD
  _bluetooth.println("SM,0");
  delay(100);
  _bluetooth.println("---");
  delay (100);
}

bool BlueSmirf::update()
{
  bool newMessage = false;
  
  while (_bluetooth.available())
  {
    newMessage |= protoUpdate();
  }

  if (!newMessage)
  {
    if (_lastMsgMs == 0)
      return false;

    unsigned long delta = millis() - _lastMsgMs;
    if (delta > 5000)
    {
      _manual = false;
      _command = Command_None;
      _rollerCommand = false;
      _valveCommand = false;
      _connected = false;

      _lastMsgMs = 0;
    }

    return false;
  }

  _lastMsgMs = millis();
  _connected = true;

  Serial.print("New message: ");
  Serial.println(_manual);
  
  // Serial.write() writes one byte
  unsigned char switches = 0x00;
  if (_left) switches |= 0x01;
  if (_front) switches |= 0x02;
  if (_aft) switches |= 0x04;
  if (_right) switches |= 0x08;

  unsigned char outputs = 0x00;
  if (_rollerStatus) outputs |= 0x01;
  if (_valveStatus) outputs |= 0x02;

  _bluetooth.write((byte)'$');
  _bluetooth.write((byte)_appStatus);
  _bluetooth.write((byte)switches);
  _bluetooth.write((byte)outputs);
  _bluetooth.write((byte)(((unsigned int)_x >> 8) & 0xFF));
  _bluetooth.write((byte)((unsigned int)_x & 0xFF));
  _bluetooth.write((byte)(((unsigned int)_y >> 8) & 0xFF));
  _bluetooth.write((byte)((unsigned int)_y & 0xFF));
  _bluetooth.write((byte)(((unsigned int)_a >> 8) & 0xFF));
  _bluetooth.write((byte)((unsigned int)_a & 0xFF));
  _bluetooth.write((byte)(((unsigned int)_b >> 8) & 0xFF));
  _bluetooth.write((byte)((unsigned int)_b & 0xFF));
  _bluetooth.write((char)_pwmL);
  _bluetooth.write((char)_pwmR);
  _bluetooth.write((byte)'#');

  return true;
}

void BlueSmirf::setSwitches(bool l, bool f, bool a, bool r)
{
  _left = l;
  _front = f;
  _aft = a;
  _right = r;
}

void BlueSmirf::setPos(float x, float y)
{
  _x = x;
  _y = y;
}

void BlueSmirf::setLengths(float a, float b)
{
  _a = a;
  _b = b;
}

void BlueSmirf::setPWM(int l, int r)
{
  _pwmL = l;
  _pwmR = r;
}

void BlueSmirf::setAppStatus(int status)
{
  _appStatus = status;
}

void BlueSmirf::setRoller(bool on)
{
  _rollerStatus = on;
}

void BlueSmirf::setValve(bool on)
{
  _valveStatus = on;
}

bool BlueSmirf::connected()
{
  return _connected;
}

bool BlueSmirf::manualMode()
{
  return _manual;
}

CommandEnum BlueSmirf::command()
{
  return _command;
}

bool BlueSmirf::rollerOn()
{
  return _rollerCommand;
}

bool BlueSmirf::valveOn()
{
  return _valveCommand;
}

bool BlueSmirf::protoUpdate()
{
  bool newMessage = false;
  char c = _bluetooth.read();

  switch (_protoStatus)
  {
    case Proto_WaitSTX:
      if (c == '$')
        _protoStatus = Proto_WaitMode;
      break;
    case Proto_WaitMode:
      _manual = (c == 1);
      _protoStatus = Proto_WaitCommand;
      break;
    case Proto_WaitCommand:
      _command = (CommandEnum)c;
      _protoStatus = Proto_WaitOutput;
      break;
    case Proto_WaitOutput:
      _rollerCommand = ((c & 0x01) != 0);
      _valveCommand = ((c & 0x02) != 0);
      _protoStatus = Proto_WaitETX;
      break;
    case Proto_WaitETX:
      newMessage = true;
      _protoStatus = Proto_WaitSTX;
      break;
  }

  return newMessage;
}
