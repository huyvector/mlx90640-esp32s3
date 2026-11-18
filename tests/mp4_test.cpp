#include "../mjpeg_mp4.hpp"
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
struct Sink:camera::Mp4Sink {
 std::FILE* f;
 explicit Sink(const char* n):f(std::fopen(n,"wb+")){assert(f);}
 ~Sink(){std::fclose(f);}
 size_t write(const uint8_t* p,size_t n)override{return std::fwrite(p,1,n,f);}
 bool seek(uint32_t p)override{return std::fseek(f,p,SEEK_SET)==0;}
 uint32_t position()override{return std::ftell(f);}
};
struct MemorySink:camera::Mp4Sink {
 std::vector<uint8_t> bytes;
 uint32_t at=0;
 bool short_write=false, fail_seek=false;
 size_t write(const uint8_t* p,size_t n)override {
  if(short_write && n)--n;
  if(bytes.size()<at+n)bytes.resize(at+n);
  std::copy(p,p+n,bytes.begin()+at);at+=n;return n;
 }
 bool seek(uint32_t p)override {if(fail_seek)return false;at=p;return true;}
 uint32_t position()override{return at;}
};
int main(int argc,char** argv) {
 {
  MemorySink s;camera::MjpegMp4 m;
  assert(m.begin(s,320,240));assert(!m.finish()); // no valid sample
  assert(m.begin(s,320,240));assert(m.beginFrame());
  const uint8_t jpeg[]={0xff,0xd8,0xff,0xd9};
  s.short_write=true;assert(!m.append(jpeg,sizeof(jpeg)));
  assert(!m.endFrame(0));assert(!m.finish()); // write failure cannot report success
  s.short_write=false;assert(m.begin(s,320,240));assert(m.beginFrame());
  assert(m.append(jpeg,sizeof(jpeg)));assert(m.endFrame(0));
  s.fail_seek=true;assert(!m.finish()); // failed index patch cannot report success
 }
 assert(argc==2);std::string dir=argv[1];
 Sink s((dir+"/test.mp4").c_str());camera::MjpegMp4 m;
 assert(m.begin(s,320,240,5));
 uint32_t pts[]={0,42,84,150,191};
 for(int i=0;i<5;++i){
  std::ifstream f(dir+"/"+std::to_string(i)+".jpg",std::ios::binary);
  std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)),{});
  assert(!b.empty());assert(m.beginFrame());assert(m.append(b.data(),b.size()));assert(m.endFrame(pts[i]));
 }
 assert(m.frames()==5);assert(!m.beginFrame());assert(m.finish());
 std::puts("PASS: MP4 index, sample limit, variable timestamps, empty/short-write/seek failures");
}
