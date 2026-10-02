#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <filesystem>

// Only validated nonzero fields used by the application, from the supplied dump.
namespace offsets {
inline constexpr wchar_t version[] = L"version-02c37bc51a384b8f";
constexpr uintptr_t fake=0x8b54980, real=0x1f8, visual=0x858d208;
constexpr uintptr_t children=0x78, children_end=8, name_container=0x70, name=8;
constexpr uintptr_t descriptor=0x18, class_name=8;
constexpr uintptr_t local_player=0x120, model=0x288, team=0x2c8, display_name=0x128;
constexpr uintptr_t primitive=0x178, position=0xd4, health=0x180, max_health=0x198;
constexpr uintptr_t dimensions=0xb10, matrix=0x1b0;
}
struct Vec3 { float x{},y{},z{}; };
struct PlayerSample {
 uintptr_t address{}, character{}, head{}, root{}, humanoid{}, team{};
 std::string name;
};
class Reader {
 HANDLE handle_{}, write_handle_{};
 DWORD pid_{};uint64_t session_{};
 uintptr_t base_{}, dm_{}, ve_{}, players_{}, local_{}, local_root_{}, local_character_{}, local_humanoid_{};
 ULONGLONG next_attach_{}, next_cache_{};
 std::vector<PlayerSample> samples_;
public:
 std::string status="Waiting for Roblox";
 ~Reader(){ close(); }
 uint64_t session()const{return session_;}
 void close(){++session_; if(write_handle_)CloseHandle(write_handle_);write_handle_=nullptr; if(handle_) CloseHandle(handle_); handle_=nullptr; pid_=0; base_=dm_=ve_=players_=local_=local_root_=local_character_=local_humanoid_=0; samples_.clear(); }
 static bool pointer(uintptr_t p){ return p>=0x10000 && p<0x0000800000000000ULL; }
 bool bytes(uintptr_t p, void* out, size_t n) const {
  SIZE_T got=0;
  return handle_ && pointer(p) && ReadProcessMemory(handle_,reinterpret_cast<LPCVOID>(p),out,n,&got) && got==n;
 }
 template<class T> T read(uintptr_t p) const { T value{}; if(!bytes(p,&value,sizeof value)) return {}; return value; }
 std::string string(uintptr_t p) const {
  if(!pointer(p)) return {};
  auto length=read<uint64_t>(p+0x10), capacity=read<uint64_t>(p+0x18);
  if(length==0 || length>128 || capacity<length || capacity>1048576) return {};
  uintptr_t data=capacity>=16?read<uintptr_t>(p):p;
  std::string result(static_cast<size_t>(length),'\0');
  if(!bytes(data,result.data(),result.size())) return {};
  for(unsigned char c:result) if(c<32 || c==127) return {};
  return result;
 }
 std::string name(uintptr_t p) const {
  auto container=read<uintptr_t>(p+offsets::name_container);
  if(!pointer(container)) return {};
  auto result=string(read<uintptr_t>(container+offsets::name));
  if(result.empty()) result=string(container);
  if(result.empty()) result=string(container+offsets::name);
  return result;
 }
 std::string class_name(uintptr_t p) const {
  auto desc=read<uintptr_t>(p+offsets::descriptor);
  return pointer(desc)?string(read<uintptr_t>(desc+offsets::class_name)):std::string{};
 }
 std::vector<uintptr_t> children(uintptr_t p) const {
  auto container=read<uintptr_t>(p+offsets::children);
  if(!pointer(container)) return {};
  auto start=read<uintptr_t>(container),end=read<uintptr_t>(container+offsets::children_end);
  if(!pointer(start) || end<start || (end-start)%16 || (end-start)>16*4096) return {};
  std::vector<uintptr_t> result;
  std::vector<uintptr_t> pairs(static_cast<size_t>((end-start)/8));
  if(!pairs.empty() && !bytes(start,pairs.data(),pairs.size()*8)) return {};
  for(size_t i=0;i<pairs.size();i+=2) if(pointer(pairs[i])) result.push_back(pairs[i]);
  return result;
 }
 uintptr_t find(uintptr_t p, const char* wanted) const {
  for(auto child:children(p)) if(name(child)==wanted) return child;
  return 0;
 }
 bool attach(){
  HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if(snap==INVALID_HANDLE_VALUE) {status="Cannot list processes";return false;}
  PROCESSENTRY32W item{};item.dwSize=sizeof item;DWORD pid=0;
  if(Process32FirstW(snap,&item)) do {if(_wcsicmp(item.szExeFile,L"RobloxPlayerBeta.exe")==0){pid=item.th32ProcessID;break;}}while(Process32NextW(snap,&item));
  CloseHandle(snap);
  if(!pid){status="Waiting for Roblox";return false;}
  handle_=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ,FALSE,pid);
  if(!handle_){status="Roblox memory is inaccessible";return false;}
  wchar_t path[32768]{};DWORD size=32768;
  if(!QueryFullProcessImageNameW(handle_,0,path,&size)){close();status="Cannot verify client version";return false;}
  if(std::filesystem::path(path).parent_path().filename()!=offsets::version){close();status="Client version differs from the supplied offsets";return false;}
  BOOL wow64=FALSE;
  if(!IsWow64Process(handle_,&wow64) || wow64){close();status="Expected a 64-bit Roblox client";return false;}
  snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
  if(snap==INVALID_HANDLE_VALUE){close();status="Cannot read Roblox module";return false;}
  MODULEENTRY32W module{};module.dwSize=sizeof module;
  if(Module32FirstW(snap,&module)) do {if(_wcsicmp(module.szModule,L"RobloxPlayerBeta.exe")==0){base_=reinterpret_cast<uintptr_t>(module.modBaseAddr);break;}}while(Module32NextW(snap,&module));
  CloseHandle(snap);pid_=pid;
  if(!base_){close();status="Roblox module unavailable";return false;}
  next_cache_=0;return true;
 }
 void update(){
  auto now=GetTickCount64();
  if(handle_){DWORD code=0;if(!GetExitCodeProcess(handle_,&code)||code!=STILL_ACTIVE){close();status="Waiting for Roblox";}}
  if(!handle_){if(now<next_attach_)return;next_attach_=now+1500;if(!attach())return;}
  auto fake=read<uintptr_t>(base_+offsets::fake);
  dm_=pointer(fake)?read<uintptr_t>(fake+offsets::real):0;
  ve_=read<uintptr_t>(base_+offsets::visual);
  if(!pointer(dm_)||!pointer(ve_)){samples_.clear();status="Waiting for a loaded game / pointers unavailable";return;}
  if(now<next_cache_)return;next_cache_=now+500;
  players_=0;
  for(auto child:children(dm_)) if(class_name(child)=="Players" || name(child)=="Players"){players_=child;break;}
  if(!players_){samples_.clear();status="Players service unavailable; check offsets";return;}
  local_=read<uintptr_t>(players_+offsets::local_player);
  local_character_=read<uintptr_t>(local_+offsets::model);local_root_=local_humanoid_=0;
  for(auto part:children(local_character_)){auto n=name(part);if(n=="HumanoidRootPart")local_root_=part;else if(n=="Humanoid")local_humanoid_=part;}
  std::vector<PlayerSample> next;
  for(auto p:children(players_)){
   if(p==local_)continue;
   PlayerSample s{};s.address=p;s.character=read<uintptr_t>(p+offsets::model);s.team=read<uintptr_t>(p+offsets::team);
   if(!pointer(s.character))continue;
   s.name=string(p+offsets::display_name);if(s.name.empty())s.name=name(p);if(s.name.empty())s.name="Player";
   for(auto part:children(s.character)){auto n=name(part);if(n=="Head")s.head=part;else if(n=="HumanoidRootPart")s.root=part;else if(n=="Humanoid")s.humanoid=part;}
   if(pointer(s.head)&&pointer(s.root))next.push_back(std::move(s));
   if(next.size()>=512)break;
  }
  samples_=std::move(next);status="Connected";
 }
 std::string execution_diagnostics() const {
  if(!handle_)return "No compatible client attached";
  if(!pointer(dm_))return "Client version matched; loaded DataModel unavailable";
  uintptr_t core=0;for(auto child:children(dm_))if(class_name(child)=="CoreGui"){core=child;break;}
  if(!pointer(core))return "Client version matched; CoreGui unavailable";
  struct Node{uintptr_t address;unsigned depth;};std::vector<Node> pending{{core,0}};size_t visited=0,modules=0;
  while(!pending.empty()&&visited<1024){auto node=pending.back();pending.pop_back();++visited;if(class_name(node.address)=="ModuleScript")++modules;
   if(node.depth<12)for(auto child:children(node.address)){if(pending.size()>=1024)break;pending.push_back({child,node.depth+1});}
  }
  return "Read-only CoreGui scan: "+std::to_string(modules)+" module scripts / "+std::to_string(visited)+" nodes (1024 node / depth 12 limit). Execution backend not connected.";
 }
 HWND window() const {
  struct Search{DWORD pid;HWND result;};Search s{pid_,nullptr};
  EnumWindows([](HWND h,LPARAM v)->BOOL{auto& s=*reinterpret_cast<Search*>(v);DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==s.pid&&IsWindowVisible(h)&&GetWindow(h,GW_OWNER)==nullptr){s.result=h;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&s));
  return s.result;
 }
 Vec3 position(uintptr_t part) const {
  auto prim=read<uintptr_t>(part+offsets::primitive);
  return pointer(prim)?read<Vec3>(prim+offsets::position):Vec3{};
 }
 uintptr_t local_character() const {return pointer(local_)?read<uintptr_t>(local_+offsets::model):0;}
 uintptr_t local_humanoid() const {return local_character()==local_character_?local_humanoid_:0;}
 uintptr_t local_primitive() const {return local_character()==local_character_?read<uintptr_t>(local_root_+offsets::primitive):0;}
 template<class T> bool write(uintptr_t address,const T& value){
  if(!handle_||!pointer(address))return false;
  if(!write_handle_)write_handle_=OpenProcess(PROCESS_VM_WRITE|PROCESS_VM_OPERATION,FALSE,pid_);
  SIZE_T done=0;return write_handle_&&WriteProcessMemory(write_handle_,reinterpret_cast<LPVOID>(address),&value,sizeof value,&done)&&done==sizeof value;
 }
 void release_write(){if(write_handle_)CloseHandle(write_handle_);write_handle_=nullptr;}
 bool try_position(uintptr_t part,Vec3& out) const {
  if(!pointer(part))return false;
  auto prim=read<uintptr_t>(part+offsets::primitive);
  return pointer(prim)&&bytes(prim+offsets::position,&out,sizeof out)&&std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z);
 }
 bool local_position(Vec3& out) const {return try_position(local_root_,out);}
 const auto& samples() const {return samples_;}
 uintptr_t local_team() const {return pointer(local_)?read<uintptr_t>(local_+offsets::team):0;}
 std::array<float,16> matrix()const{return pointer(ve_)?read<std::array<float,16>>(ve_+offsets::matrix):std::array<float,16>{};}
 std::array<float,2> dimensions()const{return pointer(ve_)?read<std::array<float,2>>(ve_+offsets::dimensions):std::array<float,2>{};}
};
inline bool project(Vec3 p,const std::array<float,16>& m,float width,float height,float& x,float& y){
 float w=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];
 if(!std::isfinite(w)||w<=0.01f)return false;
 x=(1+(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/w)*width*0.5f;
 y=(1-(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/w)*height*0.5f;
 return std::isfinite(x)&&std::isfinite(y);
}
