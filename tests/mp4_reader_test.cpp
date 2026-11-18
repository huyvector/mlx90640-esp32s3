#include "../mp4_reader.hpp"
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>
struct Source:camera::Mp4Source {
 std::vector<uint8_t> bytes;
 bool readAt(uint32_t at,uint8_t* p,size_t n)override{if(at>bytes.size()||n>bytes.size()-at)return false;std::copy(bytes.begin()+at,bytes.begin()+at+n,p);return true;}
 uint32_t size()const override{return bytes.size();}
};
int main(int argc,char** argv){assert(argc==2);Source src;std::ifstream f(argv[1],std::ios::binary);src.bytes.assign(std::istreambuf_iterator<char>(f),{});
 camera::MjpegMp4Reader r;assert(r.open(src));assert(r.count()==5&&r.duration()==232);
 const uint32_t pts[]={0,42,84,150,191};
 for(size_t i=0;i<5;++i){assert(r.sample(i).pts==pts[i]);std::vector<uint8_t> jpeg(r.sample(i).size);assert(r.read(i,jpeg.data(),jpeg.size()));}
 assert(r.frameAt(0)==0&&r.frameAt(149)==2&&r.frameAt(150)==3&&r.frameAt(99999)==4);
 auto original=src.bytes;src.bytes.pop_back();assert(!r.open(src));src.bytes=original;
 for(size_t i=0;i+16<src.bytes.size();++i)if(!std::memcmp(src.bytes.data()+i,"stco",4)){std::fill(src.bytes.begin()+i+12,src.bytes.begin()+i+16,255);break;}
 assert(!r.open(src));src.bytes.clear();assert(!r.open(src));
 std::puts("PASS: MP4 sample extraction, real timing, truncated input, invalid offsets and empty file");
}
