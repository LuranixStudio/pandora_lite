#pragma once
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>
#include <vector>
#ifndef PANDORA_BUILD_NUMBER
#define PANDORA_BUILD_NUMBER 0
#endif
namespace updater {
namespace fs=std::filesystem;
struct Result {std::string status="No update check yet";fs::path staged;std::string hash;unsigned build=0;};
inline std::future<Result> task;
inline Result state;
inline bool install=false;
struct Http {HINTERNET h{};~Http(){if(h)WinHttpCloseHandle(h);}};
inline std::vector<char> download(const wchar_t* file,size_t limit){
 Http session{WinHttpOpen(L"PandoraLiteUpdater/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,nullptr,nullptr,0)};
 if(!session.h)throw std::runtime_error("Cannot initialize update connection");
 WinHttpSetTimeouts(session.h,5000,5000,5000,10000);
 Http host{WinHttpConnect(session.h,L"github.com",INTERNET_DEFAULT_HTTPS_PORT,0)};
 Http request{host.h?WinHttpOpenRequest(host.h,L"GET",file,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
 DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
 if(!request.h||!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof policy)||!WinHttpSendRequest(request.h,nullptr,0,nullptr,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))throw std::runtime_error("Update connection failed");
 DWORD code=0,size=sizeof code;
 if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX)||code!=200)throw std::runtime_error("No published update available");
 std::vector<char> data;char buffer[16384];DWORD read=0;
 do {if(!WinHttpReadData(request.h,buffer,sizeof buffer,&read))throw std::runtime_error("Update download interrupted");if(data.size()+read>limit)throw std::runtime_error("Update exceeds size limit");data.insert(data.end(),buffer,buffer+read);}while(read);
 return data;
}
inline std::string digest(const fs::path& path){
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot read update file");
 BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
 if(BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(alg,0);throw std::runtime_error("SHA256 failed");}
 unsigned char bytes[32];char buffer[16384];bool ok=true;
 while(in){in.read(buffer,sizeof buffer);if(in.gcount()&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),static_cast<ULONG>(in.gcount()),0)<0){ok=false;break;}}
 ok=ok&&!in.bad()&&BCryptFinishHash(hash,bytes,32,0)>=0;BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
 if(!ok)throw std::runtime_error("SHA256 failed");
 const char* hex="0123456789abcdef";std::string out;for(auto b:bytes){out+=hex[b>>4];out+=hex[b&15];}return out;
}
inline bool valid_hash(const std::string& s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
inline fs::path executable(){wchar_t path[32768]{};DWORD n=GetModuleFileNameW(nullptr,path,32768);if(!n||n>=32768)throw std::runtime_error("Executable path unavailable");return fs::path(path);}
inline void check(){
 if(task.valid())return;
 state.status="Checking for updates...";
 task=std::async(std::launch::async,[]{Result out;try{
  auto manifest=download(L"/LuranixStudio/pandora_lite/releases/latest/download/release.txt",1024);
  std::istringstream input(std::string(manifest.begin(),manifest.end()));std::string extra;
  if(!(input>>out.build>>out.hash)||!valid_hash(out.hash)||(input>>extra))throw std::runtime_error("Invalid update manifest");
  if(out.build<=PANDORA_BUILD_NUMBER){out.status="You have the latest published build";return out;}
  auto data=download(L"/LuranixStudio/pandora_lite/releases/latest/download/pandora_lite.exe",32*1024*1024);
  wchar_t temp[32768]{};if(!GetTempPathW(32768,temp))throw std::runtime_error("Temporary directory unavailable");
  fs::path dir=fs::path(temp)/(L"PandoraUpdate-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
  if(!fs::create_directory(dir))throw std::runtime_error("Cannot stage update");out.staged=dir/L"pandora_lite.exe";
  {std::ofstream file(out.staged,std::ios::binary);file.write(data.data(),data.size());if(!file)throw std::runtime_error("Cannot save update");}
  DWORD type=0;if(digest(out.staged)!=out.hash||!GetBinaryTypeW(out.staged.c_str(),&type)||type!=SCS_64BIT_BINARY)throw std::runtime_error("Update verification failed");
  out.status="Verified update ready: build "+std::to_string(out.build);
 }catch(const std::exception& e){if(!out.staged.empty()){std::error_code ec;fs::remove(out.staged,ec);fs::remove(out.staged.parent_path(),ec);}out.staged.clear();out.status=e.what();}return out;});
}
inline void poll(){if(task.valid()&&task.wait_for(std::chrono::seconds(0))==std::future_status::ready)state=task.get();}
inline std::wstring quote(const std::wstring& s){return L"\""+s+L"\"";}
inline bool launch_install(){
 if(state.staged.empty())return false;
 try {if(digest(state.staged)!=state.hash)return false;
  auto args=L"--apply-update "+std::to_wstring(GetCurrentProcessId())+L" "+quote(executable().wstring())+L" "+std::wstring(state.hash.begin(),state.hash.end());
  auto result=ShellExecuteW(nullptr,L"open",state.staged.c_str(),args.c_str(),nullptr,SW_HIDE);return reinterpret_cast<INT_PTR>(result)>32;
 }catch(...){return false;}
}
inline int apply(int argc,wchar_t** argv){
 if(argc!=5)return 2;
 try {wchar_t* end=nullptr;unsigned long pid=wcstoul(argv[2],&end,10);if(!pid||*end)return 2;
  fs::path source=executable(),dest=argv[3];std::wstring wide=argv[4];std::string hash(wide.begin(),wide.end());
  if(!valid_hash(hash)||!dest.is_absolute()||dest.extension()!=L".exe"||source==dest||digest(source)!=hash)return 2;
  HANDLE original=OpenProcess(SYNCHRONIZE,FALSE,pid);if(original){auto result=WaitForSingleObject(original,30000);CloseHandle(original);if(result!=WAIT_OBJECT_0)return 3;}
  auto pending=dest;pending+=L".new";auto backup=dest;backup+=L".bak";
  if(!CopyFileW(source.c_str(),pending.c_str(),FALSE)||digest(pending)!=hash)return 4;
  if(!CopyFileW(dest.c_str(),backup.c_str(),FALSE))return 4;
  if(!MoveFileExW(pending.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return 4;
  if(reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"open",dest.c_str(),nullptr,dest.parent_path().c_str(),SW_SHOW))<=32){CopyFileW(backup.c_str(),dest.c_str(),FALSE);return 5;}return 0;
 }catch(...){return 6;}
}
}
