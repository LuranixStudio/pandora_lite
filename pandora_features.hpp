#pragma once
#include <cstdio>
#include <array>
#include <type_traits>
// Original interface and features for Pandora; no third-party cheat binaries.
inline std::string movement_status="Movement disabled";
struct Settings {
 bool enabled=true,boxes=true,names=true,health=true,teammates=false;
 bool filled=false,tracers=false,distances=false,radar=false,crosshair=false,watermark=true;
 bool speed=false,jump_boost=false,auto_jump=false,fly=false;
 int hop_interval=120,hop_duration=40;
 float walk_speed=32,jump_power=75,jump_height=12,fly_speed=40;
 bool aim=false,fov_circle=true,aim_line=true,sticky=true;
 int box_style=1,tracer_origin=2,aim_key=VK_RBUTTON,aim_part=0,profile=0,theme=0;
 float color[4]={0.55f,0.78f,0.48f,1},text_color[4]={0.94f,0.95f,0.94f,1};
 float accent[4]={0.55f,0.78f,0.48f,1},fill_color[4]={0.55f,0.78f,0.48f,0.12f};
 float thickness=1.5f,max_distance=1000,fov=120,response=140,gain=1;
 float radar_size=180,radar_range=150,radar_x=0.98f,radar_y=0.04f,cross_size=6;
 std::wstring directory,path;
 void location(){
  if(!directory.empty())return;
  wchar_t dir[32768]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",dir,32768))return;
  directory=std::wstring(dir)+L"\\PandoraLite";CreateDirectoryW(directory.c_str(),nullptr);
 }
 void fields(bool writing){
  if(path.empty())return;
  auto number=[&](const wchar_t* key,auto& value,float low,float high){
   if(writing){auto str=std::to_wstring(value);WritePrivateProfileStringW(L"Pandora",key,str.c_str(),path.c_str());}
   else{wchar_t text[64]{};auto fallback=std::to_wstring(value);GetPrivateProfileStringW(L"Pandora",key,fallback.c_str(),text,64,path.c_str());
    wchar_t* end=nullptr;double v=wcstod(text,&end);if(end!=text&&*end==0&&std::isfinite(v))value=static_cast<std::remove_reference_t<decltype(value)>>(std::clamp(v,double(low),double(high)));}
  };
#define BOOL_FIELD(v) number(L## #v,v,0,1)
#define NUM_FIELD(v,l,h) number(L## #v,v,l,h)
  BOOL_FIELD(enabled);BOOL_FIELD(boxes);BOOL_FIELD(names);BOOL_FIELD(health);BOOL_FIELD(teammates);
  BOOL_FIELD(filled);BOOL_FIELD(tracers);BOOL_FIELD(distances);BOOL_FIELD(radar);BOOL_FIELD(crosshair);BOOL_FIELD(watermark);
  BOOL_FIELD(speed);BOOL_FIELD(jump_boost);BOOL_FIELD(auto_jump);BOOL_FIELD(fly);
  NUM_FIELD(hop_interval,60,500);NUM_FIELD(hop_duration,15,120);
  NUM_FIELD(walk_speed,1,150);NUM_FIELD(jump_power,1,150);NUM_FIELD(jump_height,1,50);NUM_FIELD(fly_speed,1,150);
  BOOL_FIELD(aim);BOOL_FIELD(fov_circle);BOOL_FIELD(aim_line);BOOL_FIELD(sticky);
  NUM_FIELD(box_style,0,2);NUM_FIELD(tracer_origin,0,2);NUM_FIELD(aim_key,1,254);NUM_FIELD(aim_part,0,1);NUM_FIELD(theme,0,3);
  NUM_FIELD(thickness,1,4);NUM_FIELD(max_distance,25,5000);NUM_FIELD(fov,10,800);NUM_FIELD(response,10,500);NUM_FIELD(gain,0.1f,3);
  NUM_FIELD(radar_size,120,300);NUM_FIELD(radar_range,25,1000);NUM_FIELD(radar_x,0,1);NUM_FIELD(radar_y,0,1);NUM_FIELD(cross_size,2,20);
#undef BOOL_FIELD
#undef NUM_FIELD
  float* colors[]={color,text_color,accent,fill_color};
  for(int c=0;c<4;++c)for(int i=0;i<4;++i){auto key=L"palette"+std::to_wstring(c)+L"_"+std::to_wstring(i);number(key.c_str(),colors[c][i],0,1);}
 }
 void load(){location();path=directory+L"\\settings.ini";fields(false);}
 void save(){fields(true);}
 void saved_profile(bool writing){location();auto previous=path;path=directory+L"\\profile"+std::to_wstring(profile+1)+L".ini";fields(writing);path=previous;}
 void reset(){auto dir=directory,p=path;int slot=profile;*this=Settings{};directory=dir;path=p;profile=slot;}
};
inline ImU32 rgba(const float* c){return ImGui::ColorConvertFloat4ToU32({c[0],c[1],c[2],c[3]});}
inline void label(ImDrawList* d,ImVec2 p,ImU32 c,const char* text){d->AddText({p.x+1,p.y+1},IM_COL32(0,0,0,230),text);d->AddText(p,c,text);}
inline void player_box(ImDrawList* d,ImVec2 a,ImVec2 b,ImU32 c,float t,int type){
 if(type!=1){d->AddRect(a,b,c,type==2?5.f:0.f,0,t);return;}
 float x=(b.x-a.x)*0.25f,y=(b.y-a.y)*0.20f;
 for(int i=0;i<2;++i)for(int j=0;j<2;++j){float px=i?b.x:a.x,py=j?b.y:a.y;
  d->AddLine({px,py},{px+(i?-x:x),py},c,t);d->AddLine({px,py},{px,py+(j?-y:y)},c,t);}
}
struct TargetPoint {uintptr_t id{};float x{},y{},score{};};
inline void draw_features(Reader& r,const Settings& s,int width,int height,bool input_allowed,float dt){
 static uintptr_t lock=0;static float residual_x=0,residual_y=0;
 auto* draw=ImGui::GetBackgroundDrawList();ImVec2 center{width*.5f,height*.5f};
 if(s.watermark){draw->AddRectFilled({16,16},{258,44},IM_COL32(18,21,19,220),5);draw->AddRectFilled({16,16},{19,44},rgba(s.accent),2);char text[80];snprintf(text,sizeof text,"PANDORA  |  %.0f fps  |  %zu players",ImGui::GetIO().Framerate,r.samples().size());label(draw,{27,23},rgba(s.text_color),text);}
 if(s.crosshair){auto c=rgba(s.accent);float z=s.cross_size;draw->AddLine({center.x-z,center.y},{center.x+z,center.y},IM_COL32(0,0,0,220),3);draw->AddLine({center.x,center.y-z},{center.x,center.y+z},IM_COL32(0,0,0,220),3);draw->AddLine({center.x-z,center.y},{center.x+z,center.y},c);draw->AddLine({center.x,center.y-z},{center.x,center.y+z},c);}
 if(s.aim&&s.fov_circle)draw->AddCircle(center,s.fov,rgba(s.accent),96,1);
 bool held=s.aim&&input_allowed&&(GetAsyncKeyState(s.aim_key)&0x8000);
 if(!held){lock=0;residual_x=residual_y=0;}
 auto m=r.matrix();auto dimensions=r.dimensions();
 if(!std::isfinite(dimensions[0])||!std::isfinite(dimensions[1])||dimensions[0]<=0||dimensions[1]<=0){lock=0;residual_x=residual_y=0;return;}
 auto screen=[&](Vec3 p,float& x,float& y){if(!project(p,m,dimensions[0],dimensions[1],x,y))return false;x*=width/dimensions[0];y*=height/dimensions[1];return true;};
 Vec3 local{};bool local_valid=r.local_position(local);auto team=r.local_team();
 float radar_left=std::clamp((width-s.radar_size)*s.radar_x,0.f,std::max(0.f,width-s.radar_size));
 float radar_top=std::clamp((height-s.radar_size)*s.radar_y,0.f,std::max(0.f,height-s.radar_size));
 ImVec2 rc{radar_left+s.radar_size*.5f,radar_top+s.radar_size*.5f};float rr=s.radar_size*.5f-10;
 if(s.radar){draw->AddRectFilled({radar_left,radar_top},{radar_left+s.radar_size,radar_top+s.radar_size},IM_COL32(16,20,18,210),8);draw->AddCircle(rc,rr,rgba(s.accent),64);draw->AddCircle(rc,rr*.5f,IM_COL32(130,150,130,65),48);draw->AddLine({rc.x-rr,rc.y},{rc.x+rr,rc.y},IM_COL32(130,150,130,65));draw->AddLine({rc.x,rc.y-rr},{rc.x,rc.y+rr},IM_COL32(130,150,130,65));draw->AddCircleFilled(rc,3,IM_COL32(255,255,255,255));label(draw,{radar_left+8,radar_top+6},rgba(s.text_color),local_valid?"RADAR / north up":"RADAR / waiting for character");}
 TargetPoint best{},locked{};best.score=s.fov*s.fov;
 for(const auto& p:r.samples()){
  if(!s.teammates&&team&&p.team==team)continue;
  float hp=p.humanoid?r.read<float>(p.humanoid+offsets::health):0;
  float max=p.humanoid?r.read<float>(p.humanoid+offsets::max_health):0;
  if(p.humanoid&&(!std::isfinite(hp)||hp<=0))continue;
  Vec3 head{},root{};if(!r.try_position(p.head,head)||!r.try_position(p.root,root))continue;
  if(!std::isfinite(root.x)||!std::isfinite(root.y)||!std::isfinite(root.z)||!std::isfinite(head.x)||!std::isfinite(head.y)||!std::isfinite(head.z))continue;
  float distance=0;if(local_valid){float x=root.x-local.x,y=root.y-local.y,z=root.z-local.z;distance=std::sqrt(x*x+y*y+z*z);if(distance>s.max_distance)continue;}
  if(s.radar&&local_valid){float x=(root.x-local.x)/s.radar_range*rr,y=(root.z-local.z)/s.radar_range*rr;float len=std::sqrt(x*x+y*y);if(len>rr){x*=rr/len;y*=rr/len;}draw->AddCircleFilled({rc.x+x,rc.y+y},3,rgba(s.color));}
  if(held){float x,y;if(screen(s.aim_part==0?head:root,x,y)&&x>=0&&x<=width&&y>=0&&y<=height){float dx=x-center.x,dy=y-center.y,score=dx*dx+dy*dy;if(score<=s.fov*s.fov){TargetPoint t{p.address,x,y,score};if(score<best.score)best=t;if(p.address==lock)locked=t;}}}
  if(!s.enabled)continue;
  head.y+=.8f;root.y-=3;float tx,ty,bx,by;if(!screen(head,tx,ty)||!screen(root,bx,by))continue;
  float h=by-ty;if(h<4||h>height*2.f)continue;float cx=(tx+bx)*.5f,half=h*.25f;ImVec2 a{cx-half,ty},b{cx+half,by};
  if(b.x<0||a.x>width||b.y<0||a.y>height)continue;
  if(s.filled)draw->AddRectFilled(a,b,rgba(s.fill_color),s.box_style==2?5.f:0.f);
  if(s.boxes){player_box(draw,a,b,IM_COL32(0,0,0,230),s.thickness+2,s.box_style);player_box(draw,a,b,rgba(s.color),s.thickness,s.box_style);}
  if(s.names){auto size=ImGui::CalcTextSize(p.name.c_str());label(draw,{cx-size.x*.5f,ty-size.y-5},rgba(s.text_color),p.name.c_str());}
  if(s.distances&&local_valid){char text[48];snprintf(text,sizeof text,"%.0f studs",distance);auto size=ImGui::CalcTextSize(text);label(draw,{cx-size.x*.5f,by+4},rgba(s.text_color),text);}
  if(s.tracers){float y=s.tracer_origin==0?0.f:s.tracer_origin==1?center.y:float(height);draw->AddLine({center.x,y},{cx,by},rgba(s.color),s.thickness);}
  if(s.health&&p.humanoid&&std::isfinite(max)&&max>0){float ratio=std::clamp(hp/max,0.f,1.f);draw->AddRectFilled({a.x-7,a.y-1},{a.x-3,b.y+1},IM_COL32(10,10,15,230));draw->AddRectFilled({a.x-6,b.y-h*ratio},{a.x-4,b.y},IM_COL32(int(255*(1-ratio)),int(220*ratio),90,255));}
 }
 if(held){auto chosen=s.sticky&&locked.id?locked:best;if(!chosen.id){lock=0;residual_x=residual_y=0;return;}
  if(chosen.id!=lock)residual_x=residual_y=0;lock=chosen.id;
  if(s.aim_line)draw->AddLine(center,{chosen.x,chosen.y},rgba(s.accent),1);
  float alpha=1-std::exp(-std::clamp(dt,0.f,.05f)*1000.f/s.response);
  residual_x+=(chosen.x-center.x)*alpha*s.gain;residual_y+=(chosen.y-center.y)*alpha*s.gain;
  LONG x=static_cast<LONG>(std::clamp(residual_x,-200.f,200.f)),y=static_cast<LONG>(std::clamp(residual_y,-200.f,200.f));
  if(x||y){INPUT in{};in.type=INPUT_MOUSE;in.mi.dx=x;in.mi.dy=y;in.mi.dwFlags=MOUSEEVENTF_MOVE;if(SendInput(1,&in,sizeof in)==1){residual_x-=x;residual_y-=y;}else residual_x=residual_y=0;}
 }
}
inline void palette(Settings& s){
 const float themes[4][3]={{.55f,.78f,.48f},{.60f,.48f,.94f},{.30f,.68f,.95f},{.94f,.56f,.39f}};
 for(int i=0;i<3;++i)s.accent[i]=s.color[i]=themes[s.theme][i];
}
inline void draw_menu(Reader& r,Settings& s,float fps,bool& active){
 auto& st=ImGui::GetStyle();ImVec4 a{s.accent[0],s.accent[1],s.accent[2],1};
 st.Colors[ImGuiCol_CheckMark]=a;st.Colors[ImGuiCol_SliderGrab]=a;st.Colors[ImGuiCol_SliderGrabActive]=a;
 st.Colors[ImGuiCol_Button]={a.x*.25f,a.y*.25f,a.z*.25f,1};st.Colors[ImGuiCol_ButtonHovered]={a.x*.45f,a.y*.45f,a.z*.45f,1};st.Colors[ImGuiCol_ButtonActive]={a.x*.6f,a.y*.6f,a.z*.6f,1};
 st.Colors[ImGuiCol_WindowBg]=ImVec4(15/255.f,17/255.f,16/255.f,1);
 st.Colors[ImGuiCol_ChildBg]=ImVec4(23/255.f,25/255.f,24/255.f,1);
 st.Colors[ImGuiCol_FrameBg]=ImVec4(32/255.f,35/255.f,34/255.f,1);
 st.Colors[ImGuiCol_Border]=ImVec4(32/255.f,35/255.f,34/255.f,1);
 st.Colors[ImGuiCol_HeaderHovered]={a.x*.25f,a.y*.25f,a.z*.25f,1};
 st.WindowRounding=7;st.ChildRounding=5;st.FrameRounding=3;st.ItemSpacing={10,9};st.WindowPadding={18,16};
 ImGui::SetNextWindowPos({24,24},ImGuiCond_Once);ImGui::SetNextWindowSize({740,550},ImGuiCond_FirstUseEver);
 ImGui::SetNextWindowSizeConstraints({660,480},{1100,850});
 ImGui::Begin("Pandora / external",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
 if(menu1011::title)ImGui::PushFont(menu1011::title);ImGui::TextColored(a,"PANDORA");if(menu1011::title)ImGui::PopFont();ImGui::SameLine();ImGui::TextDisabled("EXTERNAL / CUSTOM EDITION");ImGui::SameLine(ImGui::GetWindowWidth()-175);ImGui::TextDisabled("%.0f fps",fps);
 ImGui::Separator();static int page=0;const char* tabs[]={"Aim assist","Visuals","Radar","Appearance","Profiles","Movement","Connection"};
 static bool expanded=true;static float expansion=1;
 expansion+=(float(expanded)-expansion)*(1-std::exp(-ImGui::GetIO().DeltaTime*14));
 ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,10});
 ImGui::BeginChild("sidebar",{50+105*expansion,-28},ImGuiChildFlags_Borders);ImGui::PopStyleVar();
 if(ImGui::SmallButton(expanded?"<":" >"))expanded=!expanded;
 ImGui::Spacing();const char* symbols[]={"A","B","C","D","E","C","A"};
 for(int i=0;i<7;++i){if(menu1011::tab(symbols[i],tabs[i],page==i,expansion,rgba(s.accent)))page=i;ImGui::Dummy({0,6});}
 ImGui::Spacing();if(expansion>.75f){ImGui::TextColored(r.status=="Connected"?a:ImVec4{1,.7f,.3f,1},"%s",r.status=="Connected"?"CONNECTED":"WAITING");ImGui::TextDisabled("%zu players",r.samples().size());}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("content",{0,-28},ImGuiChildFlags_Borders);
 ImGui::TextColored(a,"%s",tabs[page]);ImGui::Separator();
 if(page==0){
  ImGui::Checkbox("Enable aim assist",&s.aim);ImGui::Checkbox("Keep current target while held",&s.sticky);
  const char* keys[]={"Right mouse","Left mouse","Mouse 4","Mouse 5","Left Alt","Left Shift"};const int codes[]={VK_RBUTTON,VK_LBUTTON,VK_XBUTTON1,VK_XBUTTON2,VK_LMENU,VK_LSHIFT};int key=0;for(int i=0;i<6;++i)if(s.aim_key==codes[i])key=i;if(ImGui::Combo("Hold key",&key,keys,6))s.aim_key=codes[key];
  ImGui::Combo("Target point",&s.aim_part,"Head\0Root / torso\0");ImGui::SliderFloat("FOV radius",&s.fov,10,800,"%.0f px");
  ImGui::SliderFloat("Smoothing time",&s.response,10,500,"%.0f ms");ImGui::SliderFloat("Mouse gain",&s.gain,.1f,3,"%.2f");
  ImGui::Checkbox("Draw FOV circle",&s.fov_circle);ImGui::Checkbox("Draw target line",&s.aim_line);
  ImGui::TextWrapped("Hold the selected key with Roblox focused and the menu closed. Aims from screen center. Mouse gain depends on your game sensitivity.");
  ImGui::TextDisabled("Team and range filters are shared with Visuals.");
 }
 if(page==1){
  ImGui::Checkbox("Enable player visuals",&s.enabled);ImGui::Checkbox("Boxes",&s.boxes);ImGui::SameLine(230);ImGui::Checkbox("Filled boxes",&s.filled);
  ImGui::Combo("Box style",&s.box_style,"Full rectangle\0Corner ESP\0Rounded\0");
  ImGui::Checkbox("Names",&s.names);ImGui::SameLine(230);ImGui::Checkbox("Health bars",&s.health);
  ImGui::Checkbox("Distances",&s.distances);ImGui::SameLine(230);ImGui::Checkbox("Tracers",&s.tracers);
  if(s.tracers)ImGui::Combo("Tracer origin",&s.tracer_origin,"Top\0Center\0Bottom\0");
  ImGui::Checkbox("Include teammates",&s.teammates);ImGui::SliderFloat("Maximum range",&s.max_distance,25,5000,"%.0f studs");ImGui::SliderFloat("Line thickness",&s.thickness,1,4,"%.1f px");
  ImGui::ColorEdit4("ESP color",s.color,ImGuiColorEditFlags_NoInputs);ImGui::ColorEdit4("Fill color",s.fill_color,ImGuiColorEditFlags_NoInputs);ImGui::ColorEdit4("Text color",s.text_color,ImGuiColorEditFlags_NoInputs);
 }
 if(page==2){
  ImGui::Checkbox("Enable radar",&s.radar);ImGui::SliderFloat("Radar size",&s.radar_size,120,300,"%.0f px");ImGui::SliderFloat("Radar range",&s.radar_range,25,1000,"%.0f studs");
  ImGui::SliderFloat("Horizontal position",&s.radar_x,0,1,"%.2f");ImGui::SliderFloat("Vertical position",&s.radar_y,0,1,"%.2f");
  ImGui::TextWrapped("North-up X/Z radar centered on your character. Players beyond radar range appear at the edge. Team and maximum-range filters apply.");
 }
 if(page==3){
  if(ImGui::Combo("Theme preset",&s.theme,"Matcha green\0Lavender\0Ice blue\0Sunset\0"))palette(s);
  ImGui::ColorEdit4("Custom accent",s.accent,ImGuiColorEditFlags_NoInputs);ImGui::Checkbox("Watermark and FPS",&s.watermark);ImGui::Checkbox("Center crosshair",&s.crosshair);ImGui::SliderFloat("Crosshair size",&s.cross_size,2,20,"%.0f px");
  ImGui::Spacing();ImGui::Text("ESP preview");auto p=ImGui::GetCursorScreenPos();auto* d=ImGui::GetWindowDrawList();ImVec2 tl{p.x+75,p.y+30},br{p.x+145,p.y+170};
  if(s.filled)d->AddRectFilled(tl,br,rgba(s.fill_color));if(s.boxes)player_box(d,tl,br,rgba(s.color),s.thickness,s.box_style);if(s.names)label(d,{p.x+82,p.y+10},rgba(s.text_color),"Player");if(s.health)d->AddRectFilled({tl.x-7,tl.y+35},{tl.x-4,br.y},IM_COL32(90,220,100,255));if(s.distances)label(d,{tl.x+10,br.y+5},rgba(s.text_color),"120 studs");ImGui::Dummy({220,200});
 }
 if(page==4){
  ImGui::Combo("Profile slot",&s.profile,"Profile 1\0Profile 2\0Profile 3\0Profile 4\0Profile 5\0");
  static std::string notice;
  if(ImGui::Button("Save profile")){s.saved_profile(true);notice="Profile saved";}ImGui::SameLine();if(ImGui::Button("Load profile")){s.saved_profile(false);s.aim=s.speed=s.jump_boost=s.auto_jump=s.fly=false;notice="Profile loaded; active controls left off";}
  if(ImGui::Button("Save current settings")){s.save();notice="Current settings saved";}ImGui::SameLine();if(ImGui::Button("Reset defaults")){s.reset();notice="Defaults restored";}
  ImGui::TextWrapped("%s",notice.c_str());ImGui::TextWrapped("Five local profiles. Settings also save on exit. Aim assist and movement start disabled each launch; enable them when ready.");
 }
 if(page==5){
  ImGui::Checkbox("Custom walk speed",&s.speed);ImGui::SliderFloat("Walk speed",&s.walk_speed,1,150,"%.0f studs/s");
  ImGui::Checkbox("Custom jump strength",&s.jump_boost);ImGui::SliderFloat("Jump power",&s.jump_power,1,150,"%.0f");ImGui::SliderFloat("Jump height",&s.jump_height,1,50,"%.1f studs");
  ImGui::Checkbox("Bunny-hop while Space is held",&s.auto_jump);
  if(s.auto_jump){ImGui::SliderInt("Jump interval",&s.hop_interval,60,500,"%d ms");ImGui::SliderInt("Key press duration",&s.hop_duration,15,120,"%d ms");}
  ImGui::TextWrapped("Bunny-hop sends ordinary Space key presses. Try 120 ms interval / 40 ms duration first. Games still enforce their own jump rules.");
  ImGui::Checkbox("Velocity flight",&s.fly);ImGui::SliderFloat("Flight speed",&s.fly_speed,1,150,"%.0f studs/s");
  ImGui::TextWrapped("Flight: WASD to move, Space up, Left Ctrl down. Close the menu to activate. Movement pauses when the menu opens or Roblox loses focus.");
  ImGui::TextWrapped("Local-client controls. Games may override these values or correct your position. Jump strength uses your character's current jump mode. Values are restored when possible on disable or exit.");
  ImGui::TextWrapped("%s",movement_status.c_str());
 }
 if(page==6){
  ImGui::TextWrapped("%s",r.status.c_str());ImGui::TextWrapped("Offsets: version-02c37bc51a384b8f");
  ImGui::TextWrapped("Standard Head / HumanoidRootPart characters. Custom game rigs need an adapter. Radar and distance require your spawned character.");
  if(ImGui::Button("Reconnect"))r.close();ImGui::Spacing();
  ImGui::TextWrapped("External visuals, mouse aim assist and local movement controls. Silent aim, visibility raycasts and script execution are not implemented.");
  ImGui::TextWrapped("Michael Conors 1011 navigation and embedded fonts adapted to Dear ImGui 1.91 / DirectX 11. Original feature panels for Pandora.");
 }
 ImGui::EndChild();ImGui::Separator();ImGui::TextDisabled("INSERT / menu    END / exit");ImGui::SameLine();if(ImGui::SmallButton("Quit"))active=false;ImGui::End();
}
