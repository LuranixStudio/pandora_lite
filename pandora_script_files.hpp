#pragma once
#include <filesystem>
#include <cstdint>
#include <fstream>
#include <string>
#include <stdexcept>
namespace script_files {
namespace fs=std::filesystem;
inline constexpr size_t limit=1024*1024;
inline std::string read(const fs::path& path){
 std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open script");
 auto size=in.tellg();if(size<0||static_cast<uint64_t>(size)>limit)throw std::runtime_error("Scripts must be at most 1 MiB");
 std::string text(static_cast<size_t>(size),'\0');in.seekg(0);if(!text.empty()&&!in.read(text.data(),text.size()))throw std::runtime_error("Cannot read complete script");
 if(text.find('\0')!=std::string::npos)throw std::runtime_error("Binary files are not supported");
 if(text.starts_with("\xef\xbb\xbf"))text.erase(0,3);
 return text;
}
// Writes a sibling temporary file; replacement is handled by the caller's platform.
inline fs::path stage(const fs::path& path,const std::string& text){
 if(text.size()>limit||text.find('\0')!=std::string::npos)throw std::runtime_error("Invalid or oversized script");
 auto temp=path;temp+=L".pandora-new";
 std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create script temporary file");
 out.write(text.data(),text.size());out.flush();if(!out)throw std::runtime_error("Cannot save complete script");out.close();if(!out)throw std::runtime_error("Cannot close script file");return temp;
}
}
