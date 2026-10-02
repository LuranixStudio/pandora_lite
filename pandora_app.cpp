#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <cmath>
#include <string>
#include "pandora_reader.hpp"

static ID3D11Device* device;
static ID3D11DeviceContext* context;
static IDXGISwapChain* swapchain;
static ID3D11RenderTargetView* target;
static UINT pending_w{},pending_h{};
static bool menu=true, running=true;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
struct Settings {
 bool boxes=true,names=true,health=true,teammates=false,enabled=true;
 float color[4]={0.50f,0.42f,1.0f,1.0f},thickness=1.5f;
 std::wstring path;
 void load(){
  wchar_t dir[32768]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",dir,32768))return;
  path=std::wstring(dir)+L"\\PandoraLite";CreateDirectoryW(path.c_str(),nullptr);path+=L"\\settings.ini";
  boxes=GetPrivateProfileIntW(L"Visuals",L"Boxes",1,path.c_str())!=0;
  names=GetPrivateProfileIntW(L"Visuals",L"Names",1,path.c_str())!=0;
  health=GetPrivateProfileIntW(L"Visuals",L"Health",1,path.c_str())!=0;
  teammates=GetPrivateProfileIntW(L"Visuals",L"Teammates",0,path.c_str())!=0;
  enabled=GetPrivateProfileIntW(L"Visuals",L"Enabled",1,path.c_str())!=0;
  thickness=std::clamp(GetPrivateProfileIntW(L"Visuals",L"Thickness",15,path.c_str())/10.f,1.f,3.f);
  for(int i=0;i<4;++i){auto key=L"Color"+std::to_wstring(i);color[i]=std::clamp(GetPrivateProfileIntW(L"Visuals",key.c_str(),static_cast<int>(color[i]*255),path.c_str())/255.f,0.f,1.f);}
 }
 void save(){
  if(path.empty())return;
  auto put=[&](const wchar_t* key,int v){auto text=std::to_wstring(v);WritePrivateProfileStringW(L"Visuals",key,text.c_str(),path.c_str());};
  put(L"Boxes",boxes);put(L"Names",names);put(L"Health",health);put(L"Teammates",teammates);put(L"Enabled",enabled);put(L"Thickness",static_cast<int>(thickness*10));
  for(int i=0;i<4;++i){auto key=L"Color"+std::to_wstring(i);put(key.c_str(),static_cast<int>(color[i]*255));}
 }
};
static LRESULT CALLBACK window_proc(HWND h,UINT message,WPARAM w,LPARAM l){
 if(ImGui::GetCurrentContext() && menu && ImGui_ImplWin32_WndProcHandler(h,message,w,l))return TRUE;
 switch(message){
 case WM_SIZE:if(w!=SIZE_MINIMIZED){pending_w=LOWORD(l);pending_h=HIWORD(l);}return 0;
 case WM_SYSCOMMAND:if((w&0xfff0)==SC_KEYMENU)return 0;break;
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(h,message,w,l);
}
static bool create_target(){
 ID3D11Texture2D* back=nullptr;
 if(FAILED(swapchain->GetBuffer(0,IID_PPV_ARGS(&back))))return false;
 auto hr=device->CreateRenderTargetView(back,nullptr,&target);back->Release();return SUCCEEDED(hr);
}
static void cleanup(){
 if(target)target->Release();if(swapchain)swapchain->Release();if(context)context->Release();if(device)device->Release();
 target=nullptr;swapchain=nullptr;context=nullptr;device=nullptr;
}
static bool create_device(HWND h){
 DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
 desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=h;desc.SampleDesc.Count=1;desc.Windowed=TRUE;
 desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
 const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_0};D3D_FEATURE_LEVEL level;
 auto hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,2,D3D11_SDK_VERSION,&desc,&swapchain,&device,&level,&context);
 if(FAILED(hr))hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,levels,2,D3D11_SDK_VERSION,&desc,&swapchain,&device,&level,&context);
 return SUCCEEDED(hr)&&create_target();
}
static void style(){
 ImGui::StyleColorsDark();auto& s=ImGui::GetStyle();
 s.WindowRounding=12;s.ChildRounding=9;s.FrameRounding=6;s.GrabRounding=6;
 s.WindowPadding={24,22};s.FramePadding={10,7};s.ItemSpacing={12,12};s.WindowBorderSize=1;
 s.Colors[ImGuiCol_WindowBg]={0.055f,0.065f,0.085f,0.98f};
 s.Colors[ImGuiCol_Border]={0.16f,0.18f,0.23f,1};
 s.Colors[ImGuiCol_FrameBg]={0.10f,0.12f,0.16f,1};
 s.Colors[ImGuiCol_Button]={0.27f,0.22f,0.52f,1};
 s.Colors[ImGuiCol_ButtonHovered]={0.40f,0.32f,0.72f,1};
 s.Colors[ImGuiCol_CheckMark]={0.65f,0.55f,1,1};
 s.Colors[ImGuiCol_SliderGrab]={0.60f,0.50f,1,1};
 s.Colors[ImGuiCol_Tab]={0.12f,0.13f,0.20f,1};
 s.Colors[ImGuiCol_TabSelected]={0.28f,0.23f,0.47f,1};
}
static void draw_esp(Reader& r,const Settings& s,int width,int height){
 if(!s.enabled)return;
 auto m=r.matrix();auto d=r.dimensions();
 if(!std::isfinite(d[0])||!std::isfinite(d[1])||d[0]<=0||d[1]<=0)return;
 auto* draw=ImGui::GetBackgroundDrawList();auto team=r.local_team();
 ImU32 color=ImGui::ColorConvertFloat4ToU32({s.color[0],s.color[1],s.color[2],s.color[3]});
 for(const auto& p:r.samples()){
  if(!s.teammates&&team&&p.team==team)continue;
  float hp=p.humanoid?r.read<float>(p.humanoid+offsets::health):0;
  float max=p.humanoid?r.read<float>(p.humanoid+offsets::max_health):0;
  if(p.humanoid&&(!std::isfinite(hp)||hp<=0))continue;
  auto head=r.position(p.head),root=r.position(p.root);
  head.y+=0.8f;root.y-=3.f;
  float tx,ty,bx,by;
  if(!project(head,m,d[0],d[1],tx,ty)||!project(root,m,d[0],d[1],bx,by))continue;
  tx*=width/d[0];bx*=width/d[0];ty*=height/d[1];by*=height/d[1];
  float h=by-ty;if(h<4||h>height*2.f)continue;
  float center=(tx+bx)/2.f,half=h*0.25f;
  ImVec2 a{center-half,ty},b{center+half,by};
  if(b.x<0||a.x>width||b.y<0||a.y>height)continue;
  if(s.boxes){draw->AddRect({a.x-1,a.y-1},{b.x+1,b.y+1},IM_COL32(0,0,0,220),0,0,s.thickness+2);draw->AddRect(a,b,color,0,0,s.thickness);}
  if(s.names){auto size=ImGui::CalcTextSize(p.name.c_str());ImVec2 at{center-size.x/2,ty-size.y-6};draw->AddText({at.x+1,at.y+1},IM_COL32(0,0,0,255),p.name.c_str());draw->AddText(at,IM_COL32(240,240,250,255),p.name.c_str());}
  if(s.health&&p.humanoid&&std::isfinite(max)&&max>0){float ratio=std::clamp(hp/max,0.f,1.f);draw->AddRectFilled({a.x-7,a.y-1},{a.x-3,b.y+1},IM_COL32(10,10,15,230));draw->AddRectFilled({a.x-6,b.y-h*ratio},{a.x-4,b.y},IM_COL32(80,220,140,255));}
 }
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 WNDCLASSEXW wc{sizeof wc};wc.lpfnWndProc=window_proc;wc.hInstance=instance;wc.lpszClassName=L"PandoraLiteWindow";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
 if(!RegisterClassExW(&wc))return 1;
 HWND h=CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Pandora Lite",WS_POPUP,80,80,760,520,nullptr,nullptr,instance,nullptr);
 if(!h){UnregisterClassW(wc.lpszClassName,instance);return 1;}
 SetLayeredWindowAttributes(h,0,255,LWA_ALPHA);MARGINS margins{-1};DwmExtendFrameIntoClientArea(h,&margins);
 if(!create_device(h)){cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);MessageBoxW(nullptr,L"Unable to initialize DirectX 11.",L"Pandora Lite",MB_ICONERROR);return 1;}
 ShowWindow(h,SW_SHOW);UpdateWindow(h);IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
 style();if(!ImGui_ImplWin32_Init(h)||!ImGui_ImplDX11_Init(device,context)){ImGui::DestroyContext();cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);return 1;}
 Settings settings;settings.load();Reader reader;RECT previous{};bool old_menu=!menu;bool old_focus=true;
 while(running){
  MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);if(msg.message==WM_QUIT)running=false;}if(!running)break;
  if(GetAsyncKeyState(VK_INSERT)&1)menu=!menu;
  if(GetAsyncKeyState(VK_END)&1)break;
  reader.update();HWND game=reader.window();bool focus=!game||GetForegroundWindow()==game||GetForegroundWindow()==h;
  RECT bounds{80,80,840,600};
  if(game&&IsWindowVisible(game)&&!IsIconic(game)){GetClientRect(game,&bounds);POINT at{};ClientToScreen(game,&at);OffsetRect(&bounds,at.x,at.y);}
  if(menu!=old_menu){auto ex=GetWindowLongPtrW(h,GWL_EXSTYLE);if(menu)ex&=~(WS_EX_TRANSPARENT|WS_EX_NOACTIVATE);else ex|=WS_EX_TRANSPARENT|WS_EX_NOACTIVATE;SetWindowLongPtrW(h,GWL_EXSTYLE,ex);if(menu)SetForegroundWindow(h);old_menu=menu;}
  if(!EqualRect(&bounds,&previous)){SetWindowPos(h,HWND_TOPMOST,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOACTIVATE);previous=bounds;}
  if(focus!=old_focus){ShowWindow(h,focus?SW_SHOWNOACTIVATE:SW_HIDE);old_focus=focus;}
  if(pending_w&&pending_h){if(target){target->Release();target=nullptr;}context->OMSetRenderTargets(0,nullptr,nullptr);auto hr=swapchain->ResizeBuffers(0,pending_w,pending_h,DXGI_FORMAT_UNKNOWN,0);pending_w=pending_h=0;if(FAILED(hr)||!create_target())break;}
  if(!focus||IsIconic(game)){Sleep(30);continue;}
  ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
  RECT client{};GetClientRect(h,&client);
  if(game&&GetForegroundWindow()==game)draw_esp(reader,settings,client.right,client.bottom);
  if(menu){
   ImGui::SetNextWindowPos({24,24},ImGuiCond_Once);ImGui::SetNextWindowSize({600,440},ImGuiCond_Always);
   ImGui::Begin("Pandora Lite",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
   ImGui::TextColored({0.70f,0.62f,1,1},"PANDORA");ImGui::SameLine();ImGui::TextDisabled("LITE / x64");
   ImGui::Spacing();ImGui::TextWrapped("%s",reader.status.c_str());ImGui::TextDisabled("%zu player models | %.0f FPS",reader.samples().size(),io.Framerate);ImGui::Separator();
   if(ImGui::BeginTabBar("navigation")){
    if(ImGui::BeginTabItem("Visuals")){
     ImGui::Spacing();ImGui::Checkbox("Enable visuals",&settings.enabled);
     ImGui::Checkbox("Player boxes",&settings.boxes);ImGui::SameLine(260);ImGui::Checkbox("Player names",&settings.names);
     ImGui::Checkbox("Health bars",&settings.health);ImGui::SameLine(260);ImGui::Checkbox("Show teammates",&settings.teammates);
     ImGui::Spacing();ImGui::ColorEdit4("Box color",settings.color,ImGuiColorEditFlags_NoInputs);ImGui::SliderFloat("Line width",&settings.thickness,1,3,"%.1f px");
     ImGui::Spacing();if(ImGui::Button("Save settings"))settings.save();ImGui::SameLine();if(ImGui::Button("Reset")){auto path=settings.path;settings=Settings{};settings.path=path;}
     ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("Connection")){
     ImGui::Spacing();ImGui::TextWrapped("Supported offsets: version-02c37bc51a384b8f");
     ImGui::TextWrapped("Open Roblox and join a game. The application reconnects automatically and only enables reads for the matching client version.");
     ImGui::Spacing();ImGui::TextWrapped("Universal character support: Head, HumanoidRootPart, Humanoid. Custom character systems may need a separate adapter.");
     if(ImGui::Button("Reconnect"))reader.close();ImGui::EndTabItem();
    }
    if(ImGui::BeginTabItem("About")){
     ImGui::Spacing();ImGui::TextWrapped("Pandora Lite - clean build based on xLevitate's open source project. Interface: Dear ImGui / DirectX 11.");
     ImGui::Spacing();ImGui::TextWrapped("INSERT  Toggle menu\nEND     Exit\nSettings are saved automatically on exit.");
     ImGui::TextWrapped("Offset compatibility and in-game behavior require testing on the supplied client version.");ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
   }
   ImGui::Separator();ImGui::TextDisabled("INSERT / menu   END / exit");ImGui::SameLine();if(ImGui::Button("Quit"))running=false;
   ImGui::End();
  }
  ImGui::Render();const float clear[4]={0,0,0,0};context->OMSetRenderTargets(1,&target,nullptr);context->ClearRenderTargetView(target,clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  HRESULT hr=swapchain->Present(1,0);if(hr==DXGI_ERROR_DEVICE_REMOVED||hr==DXGI_ERROR_DEVICE_RESET)break;if(hr==DXGI_STATUS_OCCLUDED)Sleep(30);
 }
 settings.save();ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);return 0;
}
