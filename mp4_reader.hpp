#pragma once
#include "mjpeg_mp4.hpp"
namespace camera {
class Mp4Source {
 public:
 virtual ~Mp4Source()=default;
 virtual bool readAt(uint32_t offset,uint8_t* bytes,size_t n)=0;
 virtual uint32_t size() const=0;
};
// Bounded reader for single-track, non-fragmented MJPEG/MP4 clips written here.
class MjpegMp4Reader {
 public:
 struct Sample {uint32_t offset=0,size=0,duration=0,pts=0;};
 private:
 Mp4Source* source_=nullptr;
 std::vector<Sample,VideoAllocator<Sample>> samples_;
 std::vector<uint32_t,VideoAllocator<uint32_t>> durations_;
 uint32_t timescale_=0,total_=0;
 bool codec_=false,have_sizes_=false,have_offsets_=false;
 bool u32(uint32_t p,uint32_t& v) {
  uint8_t b[4];if(!source_||p>source_->size()||source_->size()-p<4||!source_->readAt(p,b,4))return false;
  v=(uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3];return true;
 }
 bool boxes(uint32_t start,uint32_t end,unsigned depth) {
  if(depth>6)return false;
  while(start<end) {
   uint32_t length,type;if(end-start<8||!u32(start,length)||!u32(start+4,type))return false;
   if(!length)length=end-start;
   if(length<8||length>end-start)return false;
   const uint32_t finish=start+length;
   if(type==0x6d6f6f76||type==0x7472616b||type==0x6d646961||type==0x6d696e66||type==0x7374626c) {
    if(!boxes(start+8,finish,depth+1))return false;
   } else if(type==0x6d646864) { // mdhd, version 0
    uint32_t version;if(length<32||!u32(start+8,version)||version>>24||!u32(start+20,timescale_)||!timescale_)return false;
   } else if(type==0x73747364) { // one JPEG sample description
    uint32_t n,tag;if(length<86||!u32(start+12,n)||n!=1||!u32(start+20,tag)||tag!=0x6a706567)return false;codec_=true;
   } else if(type==0x7374737a) {
    uint32_t fixed,n;if(have_sizes_||length<20||!u32(start+12,fixed)||!u32(start+16,n)||!n||n>7200)return false;
    if(!fixed&&length<20+4*n)return false;
    if(!samples_.empty()&&samples_.size()!=n)return false;samples_.resize(n);
    for(uint32_t i=0;i<n;++i){uint32_t bytes=fixed;if(!fixed&&!u32(start+20+4*i,bytes))return false;if(bytes<4||bytes>256*1024)return false;samples_[i].size=bytes;}
    have_sizes_=true;
   } else if(type==0x7374636f) {
    uint32_t n;if(have_offsets_||length<16||!u32(start+12,n)||!n||n>7200||length<16+4*n)return false;
    if(!samples_.empty()&&samples_.size()!=n)return false;samples_.resize(n);
    for(uint32_t i=0;i<n;++i)if(!u32(start+16+4*i,samples_[i].offset))return false;
    have_offsets_=true;
   } else if(type==0x73747473) {
    uint32_t n;if(!durations_.empty()||length<16||!u32(start+12,n)||n>7200||length<16+8*n)return false;
    for(uint32_t i=0;i<n;++i){uint32_t count,delta;if(!u32(start+16+8*i,count)||!u32(start+20+8*i,delta)||!count||!delta||count>7200-durations_.size())return false;durations_.insert(durations_.end(),count,delta);}
   } else if(type==0x73747363) { // exactly one sample per chunk
    uint32_t n,a,b,c;if(length!=28||!u32(start+12,n)||n!=1||!u32(start+16,a)||!u32(start+20,b)||!u32(start+24,c)||a!=1||b!=1||c!=1)return false;
   }
   start=finish;
  }
  return start==end;
 }
 public:
 void close(){source_=nullptr;samples_.clear();durations_.clear();total_=timescale_=0;codec_=have_sizes_=have_offsets_=false;}
 bool open(Mp4Source& source) {
  close();source_=&source;
  try {
   if(!boxes(0,source.size(),0)||!codec_||!have_sizes_||!have_offsets_||!timescale_||samples_.size()!=durations_.size()) {close();return false;}
   uint64_t units=0;
   for(size_t i=0;i<samples_.size();++i){auto& s=samples_[i];if(s.offset>source.size()||s.size>source.size()-s.offset){close();return false;}
    const uint64_t at=units*1000/timescale_;units+=durations_[i];const uint64_t finish=units*1000/timescale_;
    if(finish>UINT32_MAX||finish<=at){close();return false;}s.pts=at;s.duration=finish-at;
   }
   total_=uint32_t(units*1000/timescale_);return !samples_.empty();
  } catch(...) {close();return false;}
 }
 size_t count() const{return samples_.size();}
 uint32_t duration() const{return total_;}
 const Sample& sample(size_t i) const{return samples_.at(i);}
 bool read(size_t i,uint8_t* bytes,size_t capacity) {
  if(!source_||i>=samples_.size()||capacity<samples_[i].size)return false;
  const auto& s=samples_[i];if(!source_->readAt(s.offset,bytes,s.size))return false;
  return bytes[0]==0xff&&bytes[1]==0xd8&&bytes[s.size-2]==0xff&&bytes[s.size-1]==0xd9;
 }
 size_t frameAt(uint32_t elapsed) const {
  auto i=std::upper_bound(samples_.begin(),samples_.end(),elapsed,[](uint32_t t,const Sample& s){return t<s.pts;});
  return i==samples_.begin()?0:size_t(i-samples_.begin()-1);
 }
};
}
