#pragma once
#include <algorithm>
#include <cstdint>
// Input adapter supplies physical Space state and synthetic key events.
// Physical hold is tracked separately, so injected key-up cannot cancel the hold.
class AutoHop {
 bool down_=false;
 uint64_t next_press_{},release_at_{};
public:
 void stop(){if(down_&&!jump_input::send(false))return;down_=false;next_press_=release_at_=0;}
 bool tick(bool enabled,bool focused,uint64_t now,int interval,int duration){
  if(!enabled||!focused){stop();return true;}
  if(!jump_input::available()){stop();return false;}
  if(!jump_input::held()){stop();return true;}
  interval=std::clamp(interval,60,500);duration=std::clamp(duration,15,interval-15);
  if(down_&&now>=release_at_){if(!jump_input::send(false)){return false;}down_=false;}
  if(!down_&&now>=next_press_){
   if(!jump_input::send(true))return false;
   down_=true;release_at_=now+duration;next_press_=now+interval;
  }
  return true;
 }
};
