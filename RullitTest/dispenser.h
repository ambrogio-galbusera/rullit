#ifndef Dispenser_h
#define Dispenser_h

#define VALVE_OPEN_MS   10000
#define VALVE_CLOSED_MS 30000

class Dispenser
{
public:
	Dispenser(int pin);
        
	void init();
	bool update();

  bool valve();
  void setValve(bool on);
  
private:
  int _pin;
  int _status;
  bool _overridden;
  unsigned long _changeMs;
};

#endif
