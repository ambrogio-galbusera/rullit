#ifndef LimitSwitch_h
#define LimitSwitch_h

#define DEBOUNCE_MS 200

class LimitSwitch
{
public:
	LimitSwitch(int pin);
        
	void init();
	bool update();
  bool changed();
  void resetChanged();

  bool closed();
  bool open();
  unsigned long openMillis();

private:
  int _pin;
  int _status;
  bool _changed;
  unsigned long _changeMs;
  unsigned long _openMillis;
};

#endif
