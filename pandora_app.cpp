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
#include "menu_1011.hpp"
#include "pandora_features.hpp"
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
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 WNDCLASSEXW wc{sizeof wc};wc.lpfnWndProc=window_proc;wc.hInstance=instance;wc.lpszClassName=L"PandoraLiteWindow";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
 if(!RegisterClassExW(&wc))return 1;
 HWND h=CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Pandora Lite",WS_POPUP,80,80,840,660,nullptr,nullptr,instance,nullptr);
 if(!h){UnregisterClassW(wc.lpszClassName,instance);return 1;}
 SetLayeredWindowAttributes(h,0,255,LWA_ALPHA);MARGINS margins{-1};DwmExtendFrameIntoClientArea(h,&margins);
 if(!create_device(h)){cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);MessageBoxW(nullptr,L"Unable to initialize DirectX 11.",L"Pandora Lite",MB_ICONERROR);return 1;}
 ShowWindow(h,SW_SHOW);UpdateWindow(h);IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
 menu1011::fonts();style();if(!ImGui_ImplWin32_Init(h)||!ImGui_ImplDX11_Init(device,context)){ImGui::DestroyContext();cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);return 1;}
 Settings settings;settings.load();settings.aim=false;Reader reader;RECT previous{};bool old_menu=!menu;bool old_focus=true;
 while(running){
  MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);if(msg.message==WM_QUIT)running=false;}if(!running)break;
  if(GetAsyncKeyState(VK_INSERT)&1)menu=!menu;
  if(GetAsyncKeyState(VK_END)&1)break;
  reader.update();HWND game=reader.window();bool focus=!game||GetForegroundWindow()==game||GetForegroundWindow()==h;
  RECT bounds{80,80,920,740};
  if(game&&IsWindowVisible(game)&&!IsIconic(game)){GetClientRect(game,&bounds);POINT at{};ClientToScreen(game,&at);OffsetRect(&bounds,at.x,at.y);}
  if(menu!=old_menu){auto ex=GetWindowLongPtrW(h,GWL_EXSTYLE);if(menu)ex&=~(WS_EX_TRANSPARENT|WS_EX_NOACTIVATE);else ex|=WS_EX_TRANSPARENT|WS_EX_NOACTIVATE;SetWindowLongPtrW(h,GWL_EXSTYLE,ex);if(menu)SetForegroundWindow(h);old_menu=menu;}
  if(!EqualRect(&bounds,&previous)){SetWindowPos(h,HWND_TOPMOST,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOACTIVATE);previous=bounds;}
  if(focus!=old_focus){ShowWindow(h,focus?SW_SHOWNOACTIVATE:SW_HIDE);old_focus=focus;}
  if(pending_w&&pending_h){if(target){target->Release();target=nullptr;}context->OMSetRenderTargets(0,nullptr,nullptr);auto hr=swapchain->ResizeBuffers(0,pending_w,pending_h,DXGI_FORMAT_UNKNOWN,0);pending_w=pending_h=0;if(FAILED(hr)||!create_target())break;}
  if(!focus||IsIconic(game)){Sleep(30);continue;}
  ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
  RECT client{};GetClientRect(h,&client);
  if(game&&GetForegroundWindow()==game)draw_features(reader,settings,client.right,client.bottom,!menu,io.DeltaTime);
  if(menu)draw_menu(reader,settings,io.Framerate,running);
  ImGui::Render();const float clear[4]={0,0,0,0};context->OMSetRenderTargets(1,&target,nullptr);context->ClearRenderTargetView(target,clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  HRESULT hr=swapchain->Present(1,0);if(hr==DXGI_ERROR_DEVICE_REMOVED||hr==DXGI_ERROR_DEVICE_RESET)break;if(hr==DXGI_STATUS_OCCLUDED)Sleep(30);
 }
 settings.save();ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();cleanup();DestroyWindow(h);UnregisterClassW(wc.lpszClassName,instance);return 0;
}
