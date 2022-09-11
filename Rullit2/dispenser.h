#ifndef Dispenser_h
#define Dispenser_h

#define VALVE_OPEN_MS   10000
#define VALVE_CLOSED_MS 30000

//#define MODE_TIMER

class Dispenser
{
public:
	Dispenser(int pin);
        
	void init();
  void start();
  void stop();
	bool update();

  bool valve();
  void setValve(bool on);
  
private:
  int _pin;
  int _status;
  bool _overridden;
  bool _running;
  int _samplesNum;
  long _samplesSum;
  unsigned long _sampleMs;
  unsigned long _changeMs;
};

#endif
