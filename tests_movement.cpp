#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
using ULONGLONG=unsigned long long;
constexpr int VK_SPACE=32,VK_LCONTROL=162;
static bool space=false;
static ULONGLONG GetTickCount64(){static ULONGLONG now=1000;return now+=20;}
static int GetAsyncKeyState(int key){return key==VK_SPACE&&space?0x8000:0;}
struct Vec3{float x{},y{},z{};};
namespace offsets{constexpr uintptr_t health=0x180;}
struct Settings{bool speed=false,jump_boost=false,auto_jump=false,fly=false;int hop_interval=120,hop_duration=40;float walk_speed=32,jump_power=75,jump_height=12,fly_speed=40;};
static std::string movement_status;
namespace jump_input {
static bool ready=true;static std::vector<bool> events;
static bool available(){return ready;}
static bool held(){return space;}
static bool send(bool down){events.push_back(down);return true;}
}
class Reader {
 std::map<uintptr_t,std::vector<unsigned char>> memory;
public:
 uint64_t serial=1;uintptr_t character=0x10000,humanoid=0x20000,primitive=0x30000;int writes=0;
 static bool pointer(uintptr_t p){return p>=0x10000;}
 uint64_t session()const{return serial;}
 uintptr_t local_character()const{return character;}
 uintptr_t local_humanoid()const{return humanoid;}
 uintptr_t local_primitive()const{return primitive;}
 void release_write(){}
 std::array<float,16> matrix(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
 bool bytes(uintptr_t at,void* out,size_t count){auto i=memory.find(at);if(i==memory.end()||i->second.size()!=count)return false;std::memcpy(out,i->second.data(),count);return true;}
 template<class T>void seed(uintptr_t at,T value){std::vector<unsigned char> data(sizeof value);std::memcpy(data.data(),&value,sizeof value);memory[at]=data;}
 template<class T>bool write(uintptr_t at,const T& value){seed(at,value);++writes;return true;}
 template<class T>T get(uintptr_t at){T value{};if(!bytes(at,&value,sizeof value))throw std::runtime_error("Missing test data");return value;}
};
#include "pandora_movement.hpp"
static void expect(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 Reader r;Settings s;Movement m;
 r.seed(r.humanoid+offsets::health,100.f);r.seed(r.humanoid+movement_offsets::speed,16.f);
 r.seed(r.humanoid+movement_offsets::jump_power,50.f);r.seed(r.humanoid+movement_offsets::jump_height,7.2f);
 r.seed(r.humanoid+movement_offsets::use_jump_power,(unsigned char)1);r.seed(r.primitive+movement_offsets::velocity,Vec3{});
 s.speed=true;m.tick(r,s,true);expect(r.get<float>(r.humanoid+movement_offsets::speed)==32,"Speed applies");
 s.walk_speed=44;m.tick(r,s,true);m.tick(r,s,false);expect(r.get<float>(r.humanoid+movement_offsets::speed)==16,"Focus loss restores initial speed, even after slider changes");
 m.tick(r,s,true);r.seed(r.humanoid+movement_offsets::speed,80.f);m.stop(r);expect(r.get<float>(r.humanoid+movement_offsets::speed)==80,"Restoration must preserve a game's intervening change");
 s.speed=false;s.jump_boost=true;m.tick(r,s,true);expect(r.get<float>(r.humanoid+movement_offsets::jump_power)==75,"Power mode applies");
 r.seed(r.humanoid+movement_offsets::use_jump_power,(unsigned char)0);m.tick(r,s,true);expect(r.get<float>(r.humanoid+movement_offsets::jump_power)==50,"Mode switch restores old jump power");expect(r.get<float>(r.humanoid+movement_offsets::jump_height)==12,"Height mode applies");
 s.jump_boost=false;m.tick(r,s,true);expect(std::abs(r.get<float>(r.humanoid+movement_offsets::jump_height)-7.2f)<.001f,"Disabling restores jump height");
 s.auto_jump=true;space=true;jump_input::events.clear();int writes_before_hop=r.writes;m.tick(r,s,true);
 expect(jump_input::events==std::vector<bool>{true},"Physical hold starts a Space press");
 m.tick(r,s,true);expect(jump_input::events.size()==1,"Keep press down for its configured duration");
 m.tick(r,s,true);expect(jump_input::events==std::vector<bool>({true,false}),"Timed key release");
 expect(r.writes==writes_before_hop,"Input-only hopping must not write Humanoid memory");
 for(int i=0;i<4;++i)m.tick(r,s,true);
 expect(jump_input::events.back(),"Hold repeats after interval");
 m.tick(r,s,false);expect(!jump_input::events.back(),"Focus loss releases synthetic Space");
 size_t released_count=jump_input::events.size();space=false;m.tick(r,s,true);expect(jump_input::events.size()==released_count,"No presses after physical release");
 r.humanoid=0;space=true;m.tick(r,s,true);expect(jump_input::events.back(),"Hopping supports custom rigs without a Humanoid");m.stop(r);r.humanoid=0x20000;
 s.auto_jump=false;s.fly=true;space=true;m.tick(r,s,true);expect(r.get<Vec3>(r.primitive+movement_offsets::velocity).y==40,"Flight ascent applies");m.tick(r,s,false);expect(r.get<Vec3>(r.primitive+movement_offsets::velocity).y==0,"Pausing flight stops commanded velocity");
 s.fly=false;s.speed=true;space=false;m.tick(r,s,true);++r.serial;r.seed(r.humanoid+movement_offsets::speed,22.f);int before=r.writes;m.stop(r);expect(r.writes==before,"Do not restore into a new process session");expect(r.get<float>(r.humanoid+movement_offsets::speed)==22,"New session value survives");
 std::cout<<"Movement lifecycle checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
