#pragma once
// Only the physical Space key is tracked. Injected events are ignored by the
// hold tracker; all keyboard events continue to their normal recipients.
namespace jump_input {
inline HHOOK hook=nullptr;
inline bool physical_space=false;
inline LRESULT CALLBACK keyboard(int code,WPARAM message,LPARAM data){
 if(code==HC_ACTION){auto* key=reinterpret_cast<KBDLLHOOKSTRUCT*>(data);
  if(key->vkCode==VK_SPACE&&!(key->flags&LLKHF_INJECTED)){
   if(message==WM_KEYDOWN||message==WM_SYSKEYDOWN)physical_space=true;
   else if(message==WM_KEYUP||message==WM_SYSKEYUP)physical_space=false;
  }
 }
 return CallNextHookEx(hook,code,message,data);
}
inline bool initialize(){physical_space=(GetAsyncKeyState(VK_SPACE)&0x8000)!=0;hook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboard,GetModuleHandleW(nullptr),0);return hook!=nullptr;}
inline bool available(){return hook!=nullptr;}
inline bool held(){return physical_space;}
inline bool send(bool down){INPUT input{};input.type=INPUT_KEYBOARD;input.ki.wScan=static_cast<WORD>(MapVirtualKeyW(VK_SPACE,MAPVK_VK_TO_VSC));input.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP);return SendInput(1,&input,sizeof input)==1;}
inline void shutdown(){if(hook)UnhookWindowsHookEx(hook);hook=nullptr;physical_space=false;}
}
