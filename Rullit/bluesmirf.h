#ifndef BlueSmirf_h
#define BlueSmirf_h

typedef enum 
{
  Command_None,
  Command_Start,
  Command_Stop,
  Command_Left,
  Command_Up,
  Command_Down,
  Command_Right
} CommandEnum;

class BlueSmirf
{
public:
	BlueSmirf();
        
	void init();
	bool update();

  void setAppStatus(int status);
  void setSwitches(bool l, bool f, bool a, bool r);
  void setPos(float x, float y);
  void setLengths(float a, float b);
  void setPWM(int l, int r);
  void setRoller(bool on);
  void setValve(bool on);

  bool connected();
  bool manualMode();
  CommandEnum command();
  bool rollerOn();
  bool valveOn();
  
private:
  typedef enum {
    Proto_WaitSTX,
    Proto_WaitMode,
    Proto_WaitCommand,
    Proto_WaitOutput,
    Proto_WaitETX
  } ProtoStatusEnum;

  bool protoUpdate();
  
  bool _left;
  bool _front;
  bool _aft;
  bool _right;
  float _x;
  float _y;
  float _a;
  float _b;
  int _pwmL;
  int _pwmR;
  
  ProtoStatusEnum _protoStatus;  
  bool _connected;
  bool _manual;
  CommandEnum _command;
  int _appStatus;
  bool _valveCommand;
  bool _valveStatus;
  bool _rollerCommand;
  bool _rollerStatus;
  unsigned long _lastMsgMs;
};

#endif
