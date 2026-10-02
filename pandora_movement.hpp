#pragma once
// Local character controls. No integrity-check fields, injection or drivers.
#include "pandora_autohop.hpp"
namespace movement_offsets {
constexpr uintptr_t speed=0x1c0,jump_power=0x194,jump_height=0x190;
constexpr uintptr_t use_jump_power=0x1d0,velocity=0xe0;
}
class Movement {
 struct FloatPatch {
  uintptr_t address{};float original{},last{};bool applied=false;
  bool set(Reader& r,uintptr_t at,float value){
   if(!applied){float before=0;if(!r.bytes(at,&before,sizeof before)||!std::isfinite(before)||before<0||before>10000)return false;address=at;original=before;}
   if(!r.write(at,value))return false;
   last=value;applied=true;return true;
  }
  void restore(Reader& r){
   float current=0;if(applied&&r.bytes(address,&current,sizeof current)&&std::isfinite(current)&&std::abs(current-last)<.001f)r.write(address,original);
   *this=FloatPatch{};
  }
 } speed_,power_,height_;
 uintptr_t character_{},humanoid_{},primitive_{};uint64_t session_{};
 ULONGLONG next_tick_{};
 AutoHop hop_;
 bool flight_applied_=false;
 Vec3 last_velocity_{};
 bool same_character(Reader& r)const{return session_==r.session()&&character_&&r.local_character()==character_;}
 void stop_flight(Reader& r){
  Vec3 v{};if(flight_applied_&&r.local_primitive()==primitive_&&Reader::pointer(primitive_)&&r.bytes(primitive_+movement_offsets::velocity,&v,sizeof v)&&std::abs(v.x-last_velocity_.x)<2&&std::abs(v.z-last_velocity_.z)<2){Vec3 zero{};r.write(primitive_+movement_offsets::velocity,zero);}
  flight_applied_=false;
 }
 static Vec3 normalized(Vec3 v){float length=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);if(!std::isfinite(length)||length<.001f)return {};return {v.x/length,v.y/length,v.z/length};}
 void discard(){speed_=FloatPatch{};power_=FloatPatch{};height_=FloatPatch{};flight_applied_=false;}
public:
 void stop(Reader& r){
  hop_.stop();
  if(same_character(r)){speed_.restore(r);power_.restore(r);height_.restore(r);stop_flight(r);}else discard();
  r.release_write();next_tick_=0;
 }
 void tick(Reader& r,const Settings& s,bool focused){
  if(!focused||!(s.speed||s.jump_boost||s.auto_jump||s.fly)){stop(r);movement_status=focused?"Movement disabled":"Movement paused / menu or focus";return;}
  auto now=GetTickCount64();bool hop_ok=hop_.tick(s.auto_jump&&!s.fly,focused,now,s.hop_interval,s.hop_duration);
  if(!(s.speed||s.jump_boost||s.fly)){
   if(same_character(r)){speed_.restore(r);power_.restore(r);height_.restore(r);stop_flight(r);}else discard();
   r.release_write();movement_status=hop_ok?"Bunny-hop ready / hold Space":"Bunny-hop input unavailable";return;
  }
  if(now<next_tick_)return;
  next_tick_=now+16;
  auto character=r.local_character(),humanoid=r.local_humanoid();
  if(!Reader::pointer(character)||!Reader::pointer(humanoid)){movement_status="Waiting for a standard local character";return;}
  if(character!=character_||humanoid!=humanoid_||session_!=r.session()){
   // A replaced character/session invalidates saved addresses; never restore into it.
   discard();character_=character;humanoid_=humanoid;session_=r.session();primitive_=r.local_primitive();
  }
  float hp=0;if(!r.bytes(humanoid+offsets::health,&hp,sizeof hp)||!std::isfinite(hp)||hp<=0){speed_.restore(r);power_.restore(r);height_.restore(r);stop_flight(r);movement_status="Character unavailable; input hopping remains active";return;}
  bool ok=hop_ok;
  if(s.speed)ok=speed_.set(r,humanoid+movement_offsets::speed,s.walk_speed)&&ok;else speed_.restore(r);
  if(s.jump_boost){
   unsigned char mode=0;if(r.bytes(humanoid+movement_offsets::use_jump_power,&mode,1)&&mode<=1){
    if(mode){height_.restore(r);ok=power_.set(r,humanoid+movement_offsets::jump_power,s.jump_power)&&ok;}
    else{power_.restore(r);ok=height_.set(r,humanoid+movement_offsets::jump_height,s.jump_height)&&ok;}
   }else ok=false;
  }else{power_.restore(r);height_.restore(r);}
  if(s.fly){
   auto m=r.matrix();Vec3 right=normalized({m[0],0,m[2]}),up=normalized({m[4],m[5],m[6]});
   Vec3 camera_right=normalized({m[0],m[1],m[2]});
   Vec3 forward=normalized({up.y*camera_right.z-up.z*camera_right.y,0,up.x*camera_right.y-up.y*camera_right.x});
   if((right.x==0&&right.z==0)||(forward.x==0&&forward.z==0)){stop_flight(r);ok=false;}
   else {
    auto held=[](int key){return (GetAsyncKeyState(key)&0x8000)?1.f:0.f;};
    float side=held('D')-held('A'),front=held('W')-held('S'),vertical=held(VK_SPACE)-held(VK_LCONTROL);
    Vec3 direction=normalized({right.x*side+forward.x*front,vertical,right.z*side+forward.z*front});
    Vec3 velocity{direction.x*s.fly_speed,direction.y*s.fly_speed,direction.z*s.fly_speed};
    primitive_=r.local_primitive();if(Reader::pointer(primitive_)&&r.write(primitive_+movement_offsets::velocity,velocity)){flight_applied_=true;last_velocity_=velocity;}else ok=false;
   }
  }else stop_flight(r);
  movement_status=ok?"Movement active / local writes succeeded":"Movement write failed or character data unavailable";
  if(!ok)next_tick_=now+500;
 }
};
