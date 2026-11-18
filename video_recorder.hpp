#pragma once
#include <img_converters.h>
#include "mjpeg_mp4.hpp"
#include "sd_buffered_file.hpp"
#include <freertos/queue.h>
#include <freertos/semphr.h>

class VideoFileSink:public camera::Mp4Sink {
 public:SdBufferedFile file;
 size_t write(const uint8_t* p,size_t n)override{return file.write(p,n);}
 bool seek(uint32_t at)override{return file.seek(at);}
 uint32_t position()override{return file.position();}
};
class VideoRecorder {
 public:
 enum class State:uint8_t{Idle,Starting,Recording,Stopping};
 enum class Event:uint8_t{None,Started,Saved,Cancelled,Failed};
 private:
 enum class Kind:uint8_t{Start,Frame,Wake};
 struct Job{Kind kind;uint8_t slot;uint32_t pts;};
 uint8_t* rgb_[2]={nullptr,nullptr};uint8_t* jpeg_=nullptr;size_t jpeg_size_=0;
 static constexpr size_t jpeg_capacity_=256*1024;
 std::atomic<bool> available_[2]{{true},{true}},stop_requested_{false},failed_{false};
 std::atomic<unsigned> pending_{0};std::atomic<State> state_{State::Idle};std::atomic<Event> event_{Event::None};
 std::atomic<uint32_t> started_{0};
 QueueHandle_t queue_=nullptr;SemaphoreHandle_t mutex_=nullptr;
 VideoFileSink sink_;camera::MjpegMp4 mp4_;bool file_active_=false,start_processed_=false,ready_=false;char path_[64]{};
 static size_t jpeg_write(void* self,size_t index,const void* bytes,size_t size){
  auto& v=*static_cast<VideoRecorder*>(self);if(index>jpeg_capacity_||size>jpeg_capacity_-index)return 0;
  if(size)memcpy(v.jpeg_+index,bytes,size);v.jpeg_size_=std::max(v.jpeg_size_,index+size);return size;
 }
 bool openFile(){
  if(!sdcard_mount())return false;const int no=sdcard_fileno();if(no<=0)return false;
  snprintf(path_,sizeof(path_),"/MLX90640/mlx%04d.mp4",no);
  if(!sink_.file.open(path_))return false;
  if(!mp4_.begin(sink_,320,240,BOARD_VIDEO_MAX_FRAMES)){sink_.file.close();CAMERA_STORAGE.remove(path_);return false;}
  file_active_=true;Serial.printf("MP4 started: %s (full screen, real timestamps), ready after %ums, SD buffer %u bytes\n",path_,elapsedMs(),unsigned(camera_sd_buffer_size));return true;
 }
 bool closeFile(){
  const bool have_frames=encoded.load()>0;bool ok=have_frames&&mp4_.finish();sink_.file.flush();ok=ok&&!sink_.file.getWriteError();
  const auto bytes=sink_.file.size();sink_.file.close();file_active_=false;if(!ok)CAMERA_STORAGE.remove(path_);
  Serial.printf("MP4 %s: %s, encoded %u, dropped %u, bytes %u\n",ok?"saved":(have_frames?"failed":"cancelled"),path_,encoded.load(),dropped.load(),bytes);return ok;
 }
 void encode(const Job& job){
  bool ok=file_active_&&!failed_.load();jpeg_size_=0;uint32_t phase=micros();
  if(ok)ok=fmt2jpg_cb(rgb_[job.slot],320*240*2,320,240,PIXFORMAT_RGB565,BOARD_VIDEO_QUALITY,jpeg_write,this);
  last_jpeg_us=uint32_t(micros()-phase);phase=micros();
  if(ok)ok=jpeg_size_>=4&&jpeg_[0]==0xff&&jpeg_[1]==0xd8&&jpeg_[jpeg_size_-2]==0xff&&jpeg_[jpeg_size_-1]==0xd9;
  if(ok)ok=mp4_.beginFrame()&&mp4_.append(jpeg_,jpeg_size_)&&mp4_.endFrame(job.pts);
  last_write_us=uint32_t(micros()-phase);
  if(ok)encoded.fetch_add(1);else{failed_=true;stop_requested_=true;state_=State::Stopping;}
  available_[job.slot]=true;pending_.fetch_sub(1);
 }
 static void run(void* self){
  auto& v=*static_cast<VideoRecorder*>(self);Job job;
  while(true){
   if(xQueueReceive(v.queue_,&job,portMAX_DELAY)!=pdTRUE)continue;
   if(job.kind==Kind::Start){
    const bool ok=v.openFile();
    xSemaphoreTake(v.mutex_,portMAX_DELAY);v.start_processed_=true;
    if(ok){v.state_=v.stop_requested_?State::Stopping:State::Recording;if(!v.stop_requested_)v.event_=Event::Started;}
    else{v.failed_=true;v.stop_requested_=true;v.state_=State::Stopping;}
    xSemaphoreGive(v.mutex_);
   }else if(job.kind==Kind::Frame)v.encode(job);
   if(v.stop_requested_.load()){
    xSemaphoreTake(v.mutex_,portMAX_DELAY);
    const bool finish=v.start_processed_&&v.pending_.load()==0;
    if(finish)v.state_=State::Stopping;
    xSemaphoreGive(v.mutex_);
    if(finish){
     const bool have_frames=v.encoded.load()>0;
     const bool saved=v.file_active_&&v.closeFile();
     xSemaphoreTake(v.mutex_,portMAX_DELAY);
     if(have_frames&&!saved)v.failed_=true;
     v.event_=saved?Event::Saved:((!have_frames&&!v.failed_)?Event::Cancelled:Event::Failed);
     v.stop_requested_=false;v.state_=State::Idle;xSemaphoreGive(v.mutex_);
    }
   }
  }
 }
 public:
 std::atomic<uint32_t> encoded{0},dropped{0},last_jpeg_us{0},last_write_us{0};
 bool setup(){
  if(!camera_sd_buffer_setup())return false;
  for(auto& p:rgb_){p=static_cast<uint8_t*>(heap_caps_malloc(320*240*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!p)return false;}
  jpeg_=static_cast<uint8_t*>(heap_caps_malloc(jpeg_capacity_,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!jpeg_)return false;
  queue_=xQueueCreate(4,sizeof(Job));mutex_=xSemaphoreCreateMutex();if(!queue_||!mutex_)return false;
  jpgSetChroma(CHROMA_420);jpgSetRgb565BE(true);
  ready_=xTaskCreatePinnedToCore(run,"MP4 storage",12288,this,1,nullptr,1)==pdPASS;return ready_;
 }
 State state()const{return state_.load();}bool busy()const{return state()!=State::Idle;}
 bool active()const{return state()==State::Recording||state()==State::Starting;}
 bool failed()const{return failed_.load();}
 const char* path()const{return path_;}
 uint32_t elapsedMs()const{return millis()-started_.load();}
 Event pollEvent(){return event_.exchange(Event::None);}
 bool begin(){
  if(!ready_||!queue_||!mutex_)return false;
  xSemaphoreTake(mutex_,portMAX_DELAY);
  if(state_!=State::Idle){xSemaphoreGive(mutex_);return false;}
  event_=Event::None;failed_=false;stop_requested_=false;start_processed_=false;encoded=0;dropped=0;started_=millis();state_=State::Starting;
  const Job job{Kind::Start,0,0};const bool ok=xQueueSend(queue_,&job,0)==pdTRUE;
  if(!ok)state_=State::Idle;xSemaphoreGive(mutex_);return ok;
 }
 void requestStop(){
  if(!mutex_)return;xSemaphoreTake(mutex_,portMAX_DELAY);
  if(state_==State::Idle||state_==State::Stopping){xSemaphoreGive(mutex_);return;}
  stop_requested_=true;state_=State::Stopping;const Job job{Kind::Wake,0,0};xQueueSend(queue_,&job,0);xSemaphoreGive(mutex_);
 }
 // For an explicit reboot/diagnostic only; ordinary UI uses nonblocking requestStop.
 bool stop(){if(!busy())return encoded.load()>0;requestStop();while(busy())delay(1);return encoded.load()>0&&!failed_.load();}
 void submit(const void* pixels,uint32_t captured_ms){
  if(!mutex_||xSemaphoreTake(mutex_,0)!=pdTRUE)return;
  const auto st=state();if((st!=State::Starting&&st!=State::Recording)||stop_requested_||int32_t(captured_ms-started_.load())<0){xSemaphoreGive(mutex_);return;}
  uint8_t slot=2;
  for(uint8_t i=0;i<2;++i){bool expected=true;if(available_[i].compare_exchange_strong(expected,false)){slot=i;break;}}
  if(slot==2){dropped.fetch_add(1);xSemaphoreGive(mutex_);return;}
  pending_.fetch_add(1);const auto pts=uint32_t(captured_ms-started_.load());xSemaphoreGive(mutex_);
  memcpy(rgb_[slot],pixels,320*240*2);const Job job{Kind::Frame,slot,pts};
  if(xQueueSend(queue_,&job,0)!=pdTRUE){pending_.fetch_sub(1);available_[slot]=true;dropped.fetch_add(1);}
 }
};
inline VideoRecorder video_recorder;
