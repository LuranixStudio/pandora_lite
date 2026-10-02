#pragma once
#include <commdlg.h>
#include <deque>
#include <iomanip>
#include <misc/cpp/imgui_stdlib.h>
#include "pandora_script_files.hpp"
namespace scripts {
namespace fs=std::filesystem;
inline ImFont* code_font=nullptr;
inline void fonts(){ImFontConfig cfg;cfg.SizePixels=16;code_font=ImGui::GetIO().Fonts->AddFontDefault(&cfg);}
struct Document {int id=0;std::string text="-- Script editor / backend not connected\nprint(\"hello\")\n";fs::path path;bool dirty=false;};
struct Editor {
 std::vector<Document> documents{Document{1}};
 std::deque<std::string> logs;
 int selected=1,next_id=2,close_id=0,pending_select=0;bool initialized=false;ULONGLONG next_save=0;
 fs::path draft_path;
 void log(const std::string& message){SYSTEMTIME t{};GetLocalTime(&t);char time[24];snprintf(time,sizeof time,"[%02u:%02u:%02u] ",t.wHour,t.wMinute,t.wSecond);logs.push_back(std::string(time)+message);if(logs.size()>200)logs.pop_front();}
 static std::string utf8(const std::wstring& text){int size=WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);std::string out(size,'\0');if(size)WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size,nullptr,nullptr);return out;}
 Document* current(){for(auto& d:documents)if(d.id==selected)return &d;return nullptr;}
 static void write(const fs::path& path,const std::string& text){auto stage=script_files::stage(path,text);if(!MoveFileExW(stage.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace script; original file retained");}
 void initialize(const std::wstring& directory){if(initialized)return;initialized=true;log("Execution backend unavailable; Run is disabled.");
  if(directory.empty()){log("Draft recovery unavailable: settings folder missing");return;}
  draft_path=fs::path(directory)/L"editor-draft.luau";
  try {if(fs::exists(draft_path)){documents[0].text=script_files::read(draft_path);documents[0].dirty=true;log("Recovered the last active draft");}}catch(const std::exception& e){log(e.what());}
 }
 bool dialog_save(Document& d,bool choose=false){
  fs::path path=d.path;
  if(path.empty()||choose){wchar_t file[32768]{};if(!path.empty()){auto value=path.wstring();if(value.size()<32768)std::copy(value.begin(),value.end(),file);}else wcscpy_s(file,L"script.luau");
   OPENFILENAMEW ofn{sizeof ofn};ofn.hwndOwner=GetActiveWindow();ofn.lpstrFilter=L"Lua / Luau scripts\0*.lua;*.luau\0Text files\0*.txt\0\0";ofn.lpstrFile=file;ofn.nMaxFile=32768;ofn.lpstrDefExt=L"luau";ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
   if(!GetSaveFileNameW(&ofn)){if(CommDlgExtendedError())log("Save dialog failed");return false;}path=file;
  }
  try {write(path,d.text);d.path=path;d.dirty=false;log("Saved "+utf8(path.filename().wstring()));return true;}catch(const std::exception& e){log(e.what());return false;}
 }
 void open(){if(documents.size()>=8){log("Close a tab before opening another script (8 tab limit)");return;}
  wchar_t file[32768]{};OPENFILENAMEW ofn{sizeof ofn};ofn.hwndOwner=GetActiveWindow();ofn.lpstrFilter=L"Lua / Luau scripts\0*.lua;*.luau\0Text files\0*.txt\0All files\0*.*\0\0";ofn.lpstrFile=file;ofn.nMaxFile=32768;ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetOpenFileNameW(&ofn)){if(CommDlgExtendedError())log("Open dialog failed");return;}
  try {auto text=script_files::read(file);Document d;d.id=next_id++;d.text=std::move(text);d.path=file;selected=d.id;pending_select=d.id;documents.push_back(std::move(d));log("Opened "+utf8(fs::path(file).filename().wstring()));}catch(const std::exception& e){log(e.what());}
 }
 void checkpoint(){if(draft_path.empty())return;if(auto* d=current())try{write(draft_path,d->text);}catch(const std::exception& e){log(std::string("Draft save: ")+e.what());}}
 void tick(){if(!initialized)return;auto now=GetTickCount64();if(now>=next_save){next_save=now+5000;checkpoint();}}
 void diagnose(Reader& r){auto report=r.execution_diagnostics();log("Client: "+r.status);log(report);log("Required script bytecode and core-script fields are unresolved for version-02c37bc51a384b8f. No injection was attempted.");}
 void draw(Reader& r,const std::wstring& directory){initialize(directory);
  ImGui::TextColored({1,.72f,.3f,1},"EDITOR READY / EXECUTION UNAVAILABLE");
  if(ImGui::Button("New tab")){if(documents.size()<8){Document d;d.id=next_id++;selected=d.id;pending_select=d.id;documents.push_back(std::move(d));}else log("8 tab limit reached");}ImGui::SameLine();if(ImGui::Button("Open..."))open();ImGui::SameLine();if(ImGui::Button("Save")){if(auto* d=current())dialog_save(*d);}ImGui::SameLine();if(ImGui::Button("Save as...")){if(auto* d=current())dialog_save(*d,true);}
  bool request_close=false;
  if(ImGui::BeginTabBar("script_tabs",ImGuiTabBarFlags_Reorderable|ImGuiTabBarFlags_FittingPolicyScroll)){
   for(auto& d:documents){std::string title=d.path.empty()?"Script "+std::to_string(d.id):utf8(d.path.filename().wstring());title+="###script"+std::to_string(d.id);bool open=true;ImGuiTabItemFlags flags=d.dirty?ImGuiTabItemFlags_UnsavedDocument:ImGuiTabItemFlags_None;if(pending_select==d.id)flags|=ImGuiTabItemFlags_SetSelected;
    if(ImGui::BeginTabItem(title.c_str(),&open,flags)){selected=d.id;ImGui::PushID(d.id);
     float height=std::max(110.f,ImGui::GetContentRegionAvail().y-180);
     if(code_font)ImGui::PushFont(code_font);
     if(ImGui::InputTextMultiline("##source",&d.text,{-1,height},ImGuiInputTextFlags_AllowTabInput)){d.dirty=true;}
     if(code_font)ImGui::PopFont();
     if(d.text.size()>script_files::limit){ImGui::TextColored({1,.5f,.3f,1},"Over 1 MiB: saving disabled until shortened");}
     size_t lines=1+std::count(d.text.begin(),d.text.end(),'\n');ImGui::TextDisabled("%zu bytes / %zu lines%s",d.text.size(),lines,d.dirty?" / unsaved":"");ImGui::PopID();ImGui::EndTabItem();
    }
    if(!open){close_id=d.id;request_close=true;}
   }ImGui::EndTabBar();pending_select=0;
  }
  if(request_close)ImGui::OpenPopup("Close script?");
  if(ImGui::BeginPopupModal("Close script?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
   Document* closing=nullptr;for(auto& d:documents)if(d.id==close_id)closing=&d;
   ImGui::TextWrapped("Save this script before closing its tab?");
   bool close=false;
   if(ImGui::Button("Save and close")){if(closing&&dialog_save(*closing))close=true;}ImGui::SameLine();if(ImGui::Button("Discard tab"))close=true;ImGui::SameLine();if(ImGui::Button("Cancel")){close_id=0;ImGui::CloseCurrentPopup();}
   if(close){documents.erase(std::remove_if(documents.begin(),documents.end(),[&](const auto& d){return d.id==close_id;}),documents.end());if(documents.empty()){Document d;d.id=next_id++;documents.push_back(std::move(d));}if(!current())selected=documents.front().id;close_id=0;ImGui::CloseCurrentPopup();checkpoint();}ImGui::EndPopup();
  }
  ImGui::BeginDisabled();ImGui::Button("Run (backend required)");ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Check connection"))diagnose(r);ImGui::SameLine();if(ImGui::Button("Clear log"))logs.clear();
  ImGui::BeginChild("execution_log",{0,95},ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);bool bottom=ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-2;for(const auto& entry:logs)ImGui::TextUnformatted(entry.c_str());if(bottom)ImGui::SetScrollHereY(1);ImGui::EndChild();
  ImGui::TextDisabled("Last active tab recovers on restart. Save other tabs before exiting.");
 }
};
inline Editor editor;
}
