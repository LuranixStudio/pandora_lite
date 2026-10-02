#include "pandora_script_files.hpp"
#include <iostream>
#include <chrono>
int main(){
 namespace fs=std::filesystem;auto root=fs::temp_directory_path()/("pandora-files-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));fs::create_directory(root);
 auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
 try {
  auto original=root/"script.luau";{std::ofstream out(original,std::ios::binary);out<<"print('original')\n";}
  auto staged=script_files::stage(original,"print('updated')\n");
  check(script_files::read(original)=="print('original')\n","Staging changed the original");check(script_files::read(staged)=="print('updated')\n","Staged contents differ");
  {std::ofstream out(root/"bom.lua",std::ios::binary);out<<"\xef\xbb\xbfreturn 1";}check(script_files::read(root/"bom.lua")=="return 1","UTF8 BOM not removed");
  {std::ofstream out(root/"binary.lua",std::ios::binary);char bytes[]={'a',0,'b'};out.write(bytes,3);}bool rejected=false;try{script_files::read(root/"binary.lua");}catch(...){rejected=true;}check(rejected,"Binary file accepted");
  rejected=false;try{script_files::stage(original,std::string(script_files::limit+1,'x'));}catch(...){rejected=true;}check(rejected,"Oversized save accepted");check(script_files::read(original)=="print('original')\n","Rejected save modified original");
  {std::ofstream out(root/"huge.lua",std::ios::binary);out<<std::string(script_files::limit+1,'x');}rejected=false;try{script_files::read(root/"huge.lua");}catch(...){rejected=true;}check(rejected,"Oversized open accepted");
  rejected=false;try{script_files::read(root/"missing.lua");}catch(...){rejected=true;}check(rejected,"Missing file accepted");
  fs::remove_all(root);std::cout<<"Script open, staging, BOM, binary and size checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';fs::remove_all(root);return 1;}
}
