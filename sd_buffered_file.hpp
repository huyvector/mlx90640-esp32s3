#pragma once
#include <cstdio>
#include <cstring>
#include <atomic>
#include <unistd.h>
#include <esp_heap_caps.h>
// Writer, player and photo saver lease one reserved internal DMA buffer.
inline char* camera_sd_buffer=nullptr;
inline size_t camera_sd_buffer_size=0;
inline std::atomic<bool> camera_sd_buffer_leased{false};
inline bool camera_sd_buffer_setup(){
 if(camera_sd_buffer)return true;
 for(size_t n:{size_t(32768),size_t(16384),size_t(8192)}){
  camera_sd_buffer=static_cast<char*>(heap_caps_malloc(n,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA|MALLOC_CAP_8BIT));
  if(camera_sd_buffer){camera_sd_buffer_size=n;return true;}
 }
 return false;
}
class SdBufferedFile {
 std::FILE* file_=nullptr;bool leased_=false,error_=false,storage_held_=false;
 public:
 bool open(const char* path,const char* mode="wb+") {
  close();error_=false;if(!camera_storage.acquire(100))return false;storage_held_=true;if(!camera_sd_buffer_setup()){close();return false;}
  bool expected=false;if(!camera_sd_buffer_leased.compare_exchange_strong(expected,true)){close();return false;}leased_=true;
  char mounted[128];if(strlen(path)+8>=sizeof(mounted)){close();return false;}snprintf(mounted,sizeof(mounted),"/sdcard%s",path);
  file_=std::fopen(mounted,mode);
  if(!file_||std::setvbuf(file_,camera_sd_buffer,_IOFBF,camera_sd_buffer_size)!=0){close();return false;}return true;
 }
 explicit operator bool()const{return file_!=nullptr;}
 size_t write(const uint8_t* p,size_t n){return file_?std::fwrite(p,1,n,file_):0;}
 size_t read(uint8_t* p,size_t n){return file_?std::fread(p,1,n,file_):0;}
 bool seek(uint32_t at){return file_&&std::fseek(file_,at,SEEK_SET)==0;}
 uint32_t position()const{return file_?std::ftell(file_):0;}
 uint32_t size(){if(!file_)return 0;const auto at=position();if(std::fseek(file_,0,SEEK_END))return 0;const auto n=position();seek(at);return n;}
 void flush(){if(file_&&(std::fflush(file_)!=0||fsync(fileno(file_))!=0))error_=true;}
 bool getWriteError()const{return !file_||error_||std::ferror(file_);}
 void close(){if(file_){std::fclose(file_);file_=nullptr;}if(leased_){leased_=false;camera_sd_buffer_leased=false;}if(storage_held_){storage_held_=false;camera_storage.release();}}
 ~SdBufferedFile(){close();}
 SdBufferedFile()=default;SdBufferedFile(const SdBufferedFile&)=delete;SdBufferedFile& operator=(const SdBufferedFile&)=delete;
};
