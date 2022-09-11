#ifndef LimitSwitch_h
#define LimitSwitch_h

#define DEBOUNCE_MS 200

class LimitSwitch
{
public:
	LimitSwitch(int pin);
        
	void init();
	bool update();

  bool closed();
  bool open();

private:
  int _pin;
  int _status;
  unsigned long _changeMs;
};

#endif
