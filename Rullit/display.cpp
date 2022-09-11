# include "display.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* SWITCHES[] = {
	" L",
	" F",
	" A", 
	" R"
};	

const char* COMMANDS[] = {
  " N ",
  "Sta",
  "Sto", 
  "Lef",
  " Up",
  " Dn",
  "Rgh"
};  

const char* STATUSES[] = {
  "StartUp",
  "MovDown",
  "MovRight",
  "MovingUp",
  "Homing",
  "Completed"
};  

RullitDisplay::RullitDisplay()
	: _mode(Mode_Auto)
  , _command(0)
	, _x(0)
	, _y(0)
	, _a(0)
	, _b(0)
  , _connected(false)
  , _roller(false)
  , _valve(false)
{
	for (int i=0; i<(int)Switch_Num; i++)
 {
		_switches[i] = false;
    _prevSwitches[i] = false;
 }

 _needUpdate = true;
}
        
void RullitDisplay::init()
{
	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if(!_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
	{
		Serial.println(F("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever	
	}

	// Clear the buffer
	_display.clearDisplay();
}

void RullitDisplay::update()
{
	int x = 0;
	int dx = ((SCREEN_WIDTH / 2) / (int)Switch_Num);
  char text[10];

  if (!needUpdate())
    return;
  
  _display.clearDisplay();

	// switches	 status
	_display.setTextSize(1);      // Normal 1:1 pixel scale

	for (int i=0; i<(int)Switch_Num; i++)
	{
		if (_switches[i])
			_display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
		else
			_display.setTextColor(SSD1306_WHITE); // Draw white text

		_display.setCursor(x, 0);     // Start at top-left corner
    _display.print(SWITCHES[i]);
    
		x += dx;
	}

  if (_roller)
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  else
    _display.setTextColor(SSD1306_WHITE); // Draw white text

  _display.setCursor(SCREEN_WIDTH/2 + 6, 0);
  _display.print("Roll");

  if (_valve)
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  else
    _display.setTextColor(SSD1306_WHITE); // Draw white text

  _display.setCursor(SCREEN_WIDTH/2 + SCREEN_WIDTH/4 + 6, 0);
  _display.print("Valv");

  // position
  _display.setTextColor(SSD1306_WHITE); // Draw white text

  _display.setCursor(0, 9);
  sprintf(text, "x:%5d", (int)_x);
  _display.print(text);

  _display.setCursor(0, 17);
  sprintf(text, "y:%5d", (int)_y);
  _display.print(text);

  _display.setCursor(48, 9);
  sprintf(text, "a:%5d", (int)_a);
  _display.print(text);

  _display.setCursor(48, 17);
  sprintf(text, "b:%5d", (int)_b);
  _display.print(text);

  _display.setCursor(96, 9);
  sprintf(text, "L%c%3d", (_pwmL < 0)? '-' : '+', (int)fabs(_pwmL));
  _display.print(text);

  _display.setCursor(96, 17);
  sprintf(text, "R%c%3d", (_pwmR < 0)? '-' : '+', (int)fabs(_pwmR));
  _display.print(text);

  _display.setCursor(0, 25);    
  _display.print(STATUSES[_status]);

  _display.setCursor(60, 25);    
  if (_mode == Mode_Auto)
  {
    _display.setTextColor(SSD1306_WHITE);
    _display.print("Auto");
  }
  else
  {
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    _display.print("Man.");
  }

  _display.setCursor(96, 25);    
  _display.setTextColor(SSD1306_WHITE);
  _display.print(COMMANDS[_command]);

  _display.setCursor(120, 25);    
  if (!_connected)
  {
    _display.setTextColor(SSD1306_WHITE);
    _display.print("B");
  }
  else
  {
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    _display.print("B");
  }

  _display.display();

  updated();
}

	
void RullitDisplay::setSwitch(SwitchEnum sw, bool active)
{
	_switches[(int)sw] = active;
}

void RullitDisplay::setMode(ModeEnum mode)
{
  _needUpdate |= (_mode != mode);
  
	_mode = mode;
}

void RullitDisplay::setCommand(int command)
{
  _needUpdate |= (_command != command);
  
  _command = command;
}

void RullitDisplay::setConnected(bool connected)
{
  _needUpdate |= (_connected != connected);
  
  _connected = connected;
}

void RullitDisplay::setRoller(bool roller)
{
  _needUpdate |= (_roller != roller);
  
  _roller = roller;
}

void RullitDisplay::setValve(bool valve)
{
  _needUpdate |= (_valve != valve);
  
  _valve = valve;
}

void RullitDisplay::setStatus(int status)
{
  _needUpdate |= (_status != status);
  
  _status = status;
}

void RullitDisplay::setPos(float x, float y)
{
  _needUpdate |= ((_x != x) || (_y != y));
  
	_x = x;
	_y = y;
}

void RullitDisplay::setLengths(float a, float b)
{
  _needUpdate |= ((_a != a) || (_b != b));

	_a = a;
	_b = b;
}

void RullitDisplay::setPWM(int l, int r)
{
  _needUpdate |= ((_pwmL != l) || (_pwmR != r));

  _pwmL = l;
  _pwmR = r;
}

bool RullitDisplay::needUpdate()
{
  if (_needUpdate)
    return true;
  
  for (int i=0; i<(int)Switch_Num; i++)
  {
    if (_switches[i] != _prevSwitches[i])
      return true;
  }

  return false;
}

void RullitDisplay::updated()
{
  for (int i=0; i<(int)Switch_Num; i++)
  {
    _prevSwitches[i] = _switches[i];
  }

  _needUpdate = false;
}
