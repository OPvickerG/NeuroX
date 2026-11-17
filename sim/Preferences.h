#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
struct Preferences {                       // in-memory NVS stand-in (persists across sim "reboots" via file)
  static std::map<std::string,std::vector<uint8_t>>& store(){ static std::map<std::string,std::vector<uint8_t>> m; return m; }
  bool begin(const char*,bool){ return true; } void end(){}
  size_t getBytesLength(const char* k){ auto it=store().find(k); return it==store().end()?0:it->second.size(); }
  size_t getBytes(const char* k,void* b,size_t n){ auto it=store().find(k); if(it==store().end()||it->second.size()>n) return 0; memcpy(b,it->second.data(),it->second.size()); return it->second.size(); }
  size_t putBytes(const char* k,const void* b,size_t n){ store()[k].assign((const uint8_t*)b,(const uint8_t*)b+n); return n; }
  bool remove(const char* k){ return store().erase(k)>0; }
  size_t putUInt(const char* k,uint32_t v){ return putBytes(k,&v,4); }
  uint32_t getUInt(const char* k,uint32_t d){ uint32_t v; return getBytes(k,&v,4)==4 ? v : d; }
};
