#pragma once
#include "mp4_reader.hpp"
class VideoFileSource:public camera::Mp4Source {
 public:
 SdBufferedFile file;uint32_t bytes=0;
 bool readAt(uint32_t at,uint8_t* data,size_t n)override{return (file.position()==at||file.seek(at))&&file.read(data,n)==n;}
 uint32_t size()const override{return bytes;}
};
class Mp4Player {
 VideoFileSource source_;camera::MjpegMp4Reader reader_;
 uint8_t* jpeg_=nullptr;size_t capacity_=0,index_=0;
 bool opened_=false,playing_=false;uint32_t started_=0,last_progress_=0;
 public:
 bool open(const char* path) {
  close();source_.file.open(path,"rb");if(!source_.file)return false;
  source_.bytes=source_.file.size();if(!reader_.open(source_)){close();return false;}
  for(size_t i=0;i<reader_.count();++i)capacity_=std::max(capacity_,size_t(reader_.sample(i).size));
  jpeg_=static_cast<uint8_t*>(heap_caps_malloc(capacity_,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!jpeg_){close();return false;}opened_=true;index_=0;
  Serial.printf("MP4 player opened: %s, %u frames, %ums\n",path,unsigned(reader_.count()),reader_.duration());return true;
 }
 void close(){opened_=playing_=false;reader_.close();source_.file.close();heap_caps_free(jpeg_);jpeg_=nullptr;capacity_=0;index_=0;}
 bool isOpened()const{return opened_;} bool isTop()const{return index_==0;} bool isEnd()const{return !opened_||index_+1>=reader_.count();}
 bool playing()const{return playing_;}
 size_t frameIndex()const{return index_;}
 size_t frameCount()const{return reader_.count();}
 uint32_t currentTime()const{return opened_?reader_.sample(index_).pts:0;}
 uint32_t totalTime()const{return reader_.duration();}
 void seekFrame(size_t i){if(opened_){playing_=false;index_=std::min(i,reader_.count()-1);}}
 bool drawRegion(int x,int y,int w,int h) {
  if(!opened_||!reader_.read(index_,jpeg_,capacity_))return false;
  const bool ok=screen.drawJpg(jpeg_,reader_.sample(index_).size,x,y,w,h,0,0,w/320.0f,h/240.0f);
  screen_dirty=true;return ok;
 }
 bool draw(bool full=false) {
  if(!opened_||!reader_.read(index_,jpeg_,capacity_))return false;
  screen.setTextDatum(TL_DATUM);
  const bool ok=full?screen.drawJpg(jpeg_,reader_.sample(index_).size,0,0):screen.drawJpg(jpeg_,reader_.sample(index_).size,190,62,128,96,0,0,0.4f,0.4f);
  screen_dirty=true;return ok;
 }
 void rewind(){playing_=false;index_=0;draw();}
 void next(){playing_=false;if(!isEnd())++index_;draw();}
 void prev(){playing_=false;if(index_)--index_;draw();}
 void play(bool render=true){if(!opened_)return;if(isEnd())index_=0;started_=millis()-reader_.sample(index_).pts;playing_=true;last_progress_=0;Serial.printf("MP4 playing automatically: %u frames\n",unsigned(reader_.count()));if(render)draw(true);}
 void pause(){playing_=false;}
 void tick(){
  if(!playing_)return;
  const uint32_t elapsed=millis()-started_;
  const size_t target=reader_.frameAt(elapsed);
  if(target!=index_){index_=target;if(!draw(true)){playing_=false;Serial.println("MP4 playback read/decode failed.");}}
  if(elapsed-last_progress_>=5000){last_progress_=elapsed;Serial.printf("MP4 playback frame %u/%u, %ums\n",unsigned(index_+1),unsigned(reader_.count()),elapsed);}
  if(elapsed>=reader_.duration()){playing_=false;Serial.println("MP4 playback finished.");}
 }
};
inline Mp4Player mp4_player;
inline std::atomic<bool> board_play_last_pending{false};
