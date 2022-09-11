#ifndef RullitDisplay_h
#define RullitDisplay_h

typedef enum {
	Switch_Left,
	Switch_Front,
	Switch_Rear,
	Switch_Right,
	Switch_Num
} SwitchEnum;

typedef enum {
	Mode_Auto,
	Mode_Manual
} ModeEnum;

    

class RullitDisplay
{
public:
	RullitDisplay();
        
	void init();
	void update();
	
	void setSwitch(SwitchEnum sw, bool active);
	void setMode(ModeEnum mode);
  void setCommand(int command);
  void setConnected(bool connected);
	void setPos(float x, float y);
	void setLengths(float a, float b);   
  void setPWM(int l, int r);
  void setStatus(int status);
  void setRoller(bool on);
  void setValve(bool on);
  
private:
  bool needUpdate();
  void updated();
  
	bool _switches[(int)Switch_Num];
  bool _prevSwitches[(int)Switch_Num];
	ModeEnum _mode;
  int _command;
	float _x;
	float _y;
	float _a;
	float _b;
  int _pwmL;
  int _pwmR;
  int _status;
  bool _connected;
  bool _roller;
  bool _valve;
  bool _needUpdate;
};

#endif
