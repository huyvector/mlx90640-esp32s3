#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <limits>
#ifdef ARDUINO
#include <esp_heap_caps.h>
template<class T> struct VideoAllocator {
  using value_type = T;
  VideoAllocator() = default;
  template<class U> VideoAllocator(const VideoAllocator<U>&) {}
  T* allocate(size_t n) {
    auto p = static_cast<T*>(heap_caps_malloc(n*sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!p) throw std::bad_alloc();
    return p;
  }
  void deallocate(T* p, size_t) { heap_caps_free(p); }
  template<class U> bool operator==(const VideoAllocator<U>&) const { return true; }
  template<class U> bool operator!=(const VideoAllocator<U>&) const { return false; }
};
#else
template<class T> using VideoAllocator = std::allocator<T>;
#endif

// Minimal single-track MJPEG/MP4 muxer. Stores real capture timestamps (VFR).
// No frame duplication and no fixed-FPS duration assumptions.
namespace camera {
class Mp4Sink {
 public:
  virtual ~Mp4Sink() = default;
  virtual size_t write(const uint8_t*, size_t) = 0;
  virtual bool seek(uint32_t) = 0;
  virtual uint32_t position() = 0;
};

class MjpegMp4 {
  struct Sample { uint32_t offset, size, pts; };
  using Bytes = std::vector<uint8_t, VideoAllocator<uint8_t>>;
  std::vector<Sample, VideoAllocator<Sample>> samples_;
  Mp4Sink* sink_ = nullptr;
  uint32_t mdat_ = 0, frame_start_ = 0, max_frames_ = 0;
  uint16_t width_ = 0, height_ = 0;
  bool error_ = false;
  static void u16(Bytes& b, uint16_t v) { b.push_back(v>>8); b.push_back(v); }
  static void u32(Bytes& b, uint32_t v) { b.push_back(v>>24); b.push_back(v>>16); b.push_back(v>>8); b.push_back(v); }
  static void text(Bytes& b, const char* p, size_t n) { b.insert(b.end(),p,p+n); }
  static void zeros(Bytes& b, size_t n) { b.insert(b.end(),n,0); }
  static size_t box(Bytes& b, const char* type) { size_t p=b.size(); u32(b,0); text(b,type,4); return p; }
  static void end(Bytes& b, size_t p) { uint32_t n=b.size()-p; for(int i=0;i<4;++i)b[p+i]=n>>(24-8*i); }
  static void matrix(Bytes& b) {
    const uint32_t v[]={0x10000,0,0,0,0x10000,0,0,0,0x40000000};
    for(auto x:v)u32(b,x);
  }
  bool put(const uint8_t* p, size_t n) {
    if (!sink_ || sink_->write(p,n)!=n) { error_=true; return false; }
    return true;
  }
  uint32_t delta(size_t i) const {
    if(i+1<samples_.size()) return std::max(uint32_t(1),samples_[i+1].pts-samples_[i].pts);
    if(i) return std::max(uint32_t(1),samples_[i].pts-samples_[i-1].pts);
    return 42;
  }
 public:
  bool begin(Mp4Sink& sink, uint16_t w, uint16_t h, uint32_t max_frames=7200) {
    sink_=&sink; width_=w; height_=h; max_frames_=max_frames; error_=false;
    samples_.clear();
    try { samples_.reserve(max_frames); } catch(...) { error_=true; return false; }
    Bytes b;
    auto f=box(b,"ftyp"); text(b,"isom",4); u32(b,0x200);
    text(b,"isomiso2mp41",12); end(b,f);
    if(!put(b.data(),b.size())) return false;
    mdat_=sink_->position(); b.clear(); u32(b,0); text(b,"mdat",4);
    return put(b.data(),b.size());
  }
  bool beginFrame() {
    if(error_ || !sink_ || samples_.size()>=max_frames_) return false;
    frame_start_=sink_->position(); return frame_start_<0x7F000000u;
  }
  bool append(const uint8_t* p, size_t n) { return put(p,n); }
  bool endFrame(uint32_t pts_ms, bool encoded=true) {
    if(!encoded || error_) return false;
    const uint32_t n=sink_->position()-frame_start_;
    if(!n || (!samples_.empty() && pts_ms<=samples_.back().pts)) return false;
    samples_.push_back({frame_start_,n,pts_ms}); return true;
  }
  size_t frames() const { return samples_.size(); }
  bool finish() {
    if(!sink_ || samples_.empty() || error_) return false;
    const uint32_t media_end=sink_->position();
    if(!sink_->seek(mdat_)) return false;
    Bytes b; u32(b,media_end-mdat_);
    if(!put(b.data(),4) || !sink_->seek(media_end)) return false;
    b.clear();
    const uint32_t duration=samples_.back().pts-samples_[0].pts+delta(samples_.size()-1);
    try {
      b.reserve(samples_.size()*16+1024);
      auto moov=box(b,"moov");
      auto mvhd=box(b,"mvhd"); u32(b,0); zeros(b,8); u32(b,1000); u32(b,duration);
      u32(b,0x10000); u16(b,0x100); zeros(b,10); matrix(b); zeros(b,24); u32(b,2); end(b,mvhd);
      auto trak=box(b,"trak");
      auto tkhd=box(b,"tkhd"); u32(b,7); zeros(b,8); u32(b,1); u32(b,0); u32(b,duration);
      zeros(b,16); matrix(b); u32(b,uint32_t(width_)<<16); u32(b,uint32_t(height_)<<16); end(b,tkhd);
      auto mdia=box(b,"mdia");
      auto mdhd=box(b,"mdhd"); u32(b,0); zeros(b,8); u32(b,1000); u32(b,duration);
      u16(b,0x55C4); u16(b,0); end(b,mdhd);
      auto hdlr=box(b,"hdlr"); u32(b,0); u32(b,0); text(b,"vide",4); zeros(b,12);
      text(b,"Thermal camera",15); end(b,hdlr);
      auto minf=box(b,"minf");
      auto vmhd=box(b,"vmhd"); u32(b,1); zeros(b,8); end(b,vmhd);
      auto dinf=box(b,"dinf"); auto dref=box(b,"dref"); u32(b,0); u32(b,1);
      auto url=box(b,"url "); u32(b,1); end(b,url); end(b,dref); end(b,dinf);
      auto stbl=box(b,"stbl");
      auto stsd=box(b,"stsd"); u32(b,0); u32(b,1);
      auto jpeg=box(b,"jpeg"); zeros(b,6); u16(b,1); zeros(b,16);
      u16(b,width_); u16(b,height_); u32(b,0x00480000); u32(b,0x00480000); u32(b,0); u16(b,1);
      b.push_back(10); text(b,"MotionJPEG",10); zeros(b,21); u16(b,24); u16(b,0xFFFF);
      end(b,jpeg); end(b,stsd);
      auto stts=box(b,"stts"); u32(b,0);
      uint32_t runs=0;
      for(size_t i=0;i<samples_.size();++i) if(!i || delta(i)!=delta(i-1))++runs;
      u32(b,runs);
      for(size_t i=0;i<samples_.size();) {
        uint32_t d=delta(i),n=1;
        while(i+n<samples_.size() && delta(i+n)==d)++n;
        u32(b,n); u32(b,d); i+=n;
      }
      end(b,stts);
      auto stsc=box(b,"stsc"); u32(b,0); u32(b,1); u32(b,1); u32(b,1); u32(b,1); end(b,stsc);
      auto stsz=box(b,"stsz"); u32(b,0); u32(b,0); u32(b,samples_.size());
      for(const auto& s:samples_)u32(b,s.size); end(b,stsz);
      auto stco=box(b,"stco"); u32(b,0); u32(b,samples_.size());
      for(const auto& s:samples_)u32(b,s.offset); end(b,stco);
      end(b,stbl); end(b,minf); end(b,mdia); end(b,trak); end(b,moov);
    } catch(...) { return false; }
    bool ok=put(b.data(),b.size()); sink_=nullptr; return ok;
  }
};
} // namespace camera
