#pragma once
#include <vector>
#include <algorithm>
#include "ui_icons.hpp"
#include "ui_typography.hpp"
#include "hidden_gesture.hpp"
#include "doom_control.hpp"

namespace modern {
constexpr uint16_t rgb(unsigned r,unsigned g,unsigned b){return ((r&248)<<8)|((g&252)<<3)|(b>>3);}
constexpr auto BG=rgb(7,14,28),PANEL=rgb(12,26,46),CARD=rgb(20,44,73),LINE=rgb(31,67,99);
constexpr auto TEXT=rgb(229,246,255),MUTED=rgb(143,174,207),ACCENT=rgb(75,223,255),RED=rgb(255,108,138),AMBER=rgb(255,201,115);
enum class Page:uint8_t{Live,Settings,Grid,Image,Range,Capture,More,Touch,Info,Files,Preview,Player,Confirm,TouchTest,Units,Storage,FormatConfirm,FormatBusy,GameLoading};
enum Action:uint16_t {None,Live,Files,Settings,Back,Apply,OpenGrid,OpenImage,OpenRange,OpenCapture,OpenMore,OpenTouch,OpenInfo,Reset,
 InterpPrev,InterpNext,BoxPrev,BoxNext,Palette0,Palette1,Markers,Spot,Auto,MinMinus,MinPlus,MaxMinus,MaxPlus,Photo,Video, TouchXMinus,TouchXPlus,TouchYMinus,TouchYPlus,TouchReset,CaptureNow,Pick,FilePrevPage,FileNextPage,Select,All,Delete, File0=100,File1,File2,File3,MediaRewind,MediaPrev,MediaNext,MediaPlay,TogglePlay,ConfirmYes,ConfirmNo,InterpSlider=200,BoxSlider,LowSlider,HighSlider,OffsetXSlider,OffsetYSlider,FpsSlider,FpsPrev,FpsNext,TouchTestStart,OpenUnits,UnitC,UnitF,UnitK,OpenStorage,FormatCard,FormatYes,FormatCancel,EjectCard,RetryCard};
struct Rect {int x,y,w,h;bool contains(uint16_t px,uint16_t py)const{return camera::touch_inside(px,py,x,y,w,h);}};
struct Hit {Action action;Rect rect;};

class UI {
 Page page_=Page::Live,return_page_=Page::Settings;
 Hit hits_[32]{};unsigned hit_count_=0;Action pressed_=None;bool press_executed_=false;
 bool redraw_=true,valid_frame_=false,animating_=false,selecting_=false,reset_confirm_=false,files_loading_=false;
 uint16_t* slide_old_=nullptr;uint16_t* slide_new_=nullptr;
 unsigned anim_step_=0;uint32_t anim_tick_=0;bool backwards_=false;
 camera::Settings draft_{};camera::HiddenInfoGesture info_gesture_;
 int touch_x_=0,touch_y_=0;
 float frame_[768]{},raw_[768]{};
 int32_t levels_[768]{};
 char hud_labels_[6][32]{};int hud_state_=-1,hud_mode_=-1,hud_pulse_=-99,hud_spinner_=-1,hud_low_=-999,hud_high_=-999,hud_palette_=-1,hud_spot_=-1;
 bool hud_pressed_=false;uint32_t hud_anim_ms_=0;
 Rect heat_{4,24,240,180};Rect measurement_labels_[3]{};unsigned measurement_label_count_=0;
 std::vector<FileInfo_t> files_;
 unsigned file_page_=0;uint32_t sd_total_mb_=0,sd_free_mb_=0;
 char media_path_[96]{},toast_[80]{};uint32_t toast_until_=0,record_start_=0;
 enum class Media {None,Photo,Mp4,Raw} media_=Media::None;
 File raw_file_;uint32_t raw_count_=0,raw_index_=0,raw_started_=0;bool raw_playing_=false;
 uint32_t press_start_=0,repeat_tick_=0;
 camera::TouchSample test_samples_[6]{};
 unsigned test_point_=0,test_count_=0;uint32_t test_started_=0,test_tick_=0;
 uint32_t test_sum_x_=0,test_sum_y_=0,test_ui_x_=0,test_ui_y_=0;
 uint16_t test_min_x_=319,test_max_x_=0,test_min_y_=239,test_max_y_=0;
 bool test_held_=false,test_ready_=false;
 uint32_t format_epoch_=0,storage_seen_epoch_=0;bool automatic_format_prompt_=false,raw_storage_held_=false;camera::StorageState last_storage_state_=camera::StorageState::Absent;
 static constexpr uint16_t test_targets_[6][2]={{40,38},{160,38},{280,38},{160,90},{160,150},{160,208}};

 enum class Glyph:uint8_t{None,Camera,Video,Folder,Gear,Grid,Palette,Temp,Touch,Info,Trash,Play,Pause,Back,Check,Prev,Next,Rewind,Reset,Stop,Storage,Format,Eject,Unit,Tune,Min,Max,Spot,Warning,Photo};
 Glyph glyph(Action a)const{
  switch(a){case Live:case Photo:return Glyph::Camera;case Files:return Glyph::Folder;case Settings:case OpenMore:return Glyph::Gear;
   case OpenGrid:return Glyph::Grid;case OpenImage:case Palette0:case Palette1:return Glyph::Palette;case OpenRange:case Auto:return Glyph::Temp;case OpenCapture:case Video:return Glyph::Video;
   case OpenUnits:case UnitC:case UnitF:case UnitK:return Glyph::Unit;case OpenStorage:return Glyph::Storage;case FormatCard:case FormatYes:return Glyph::Format;case EjectCard:return Glyph::Eject;case RetryCard:return Glyph::Reset;case OpenTouch:case Markers:case Spot:return Glyph::Touch;case OpenInfo:return Glyph::Info;case Delete:return Glyph::Trash;case MediaPlay:return Glyph::Play;
   case TogglePlay:return mediaPlaying()?Glyph::Pause:Glyph::Play;case Back:return Glyph::Back;case Apply:case Select:case All:return Glyph::Check;
   case FilePrevPage:case MediaPrev:return Glyph::Prev;case FileNextPage:case MediaNext:return Glyph::Next;case MediaRewind:return Glyph::Rewind;case Reset:case TouchReset:return Glyph::Reset;default:return Glyph::None;}
 }
 void icon(Glyph g,int x,int y,uint16_t c,int scale=1){
  const int theme=c==ACCENT?1:(c==MUTED?2:(c==RED?3:(c==BG?4:0)));
  const int size=scale==2?2:(scale==0?1:0);
  const auto& asset=modern_icon_assets::icons[unsigned(g)][theme][size];
  if(asset.data)screen.drawPng(asset.data,asset.size,x,y);
 }

 void text(const char* s,int x,int y,uint16_t color=TEXT,bool small=false,lgfx::textdatum_t align=TL_DATUM) {
  screen.setTextDatum(align);screen.setTextColor(color);screen.setTextSize(1);
  if(modern_type::ready)screen.setFont(small?&modern_type::caption:&modern_type::body);else screen.setFont(small?static_cast<const lgfx::IFont*>(&fonts::Font0):static_cast<const lgfx::IFont*>(&fonts::Font2));screen.drawString(s,x,y);
 }
 float temperature(float c)const{return camera::display_temperature(c,board_temperature_unit.load());}
 const char* unitSuffix()const{return board_temperature_unit==1?"°F":(board_temperature_unit==2?"K":"°C");}
 const char* draftSuffix()const{return draft_.unit==1?"°F":(draft_.unit==2?"K":"°C");}
 void number(const char* s,int x,int y,uint16_t c){screen.setTextDatum(TL_DATUM);screen.setTextColor(c);screen.setTextSize(1);screen.setFont(modern_type::ready?static_cast<const lgfx::IFont*>(&modern_type::number):static_cast<const lgfx::IFont*>(&fonts::Font4));screen.drawString(s,x,y);}
 void add(Action a,Rect r){if(hit_count_<32)hits_[hit_count_++]={a,r};}
 Action hit(uint16_t x,uint16_t y)const{if(toast_[0]&&int32_t(toast_until_-millis())>0&&toastRect().contains(x,y))return None;for(unsigned i=0;i<hit_count_;++i)if(hits_[i].rect.contains(x,y))return hits_[i].action;return None;}
 static uint16_t blend(uint16_t a,uint16_t b,unsigned amount){const unsigned inv=255-amount;return ((((a>>11)*inv+(b>>11)*amount)/255)<<11)|(((((a>>5)&63)*inv+((b>>5)&63)*amount)/255)<<5)|(((a&31)*inv+(b&31)*amount)/255);}
 void glowPanel(Rect r,uint16_t base,uint16_t edge,bool active=false,int radius=8){
  radius=std::max(1,std::min(radius,std::min(r.w,r.h)/2));
  for(int y=0;y<r.h;++y){int inset=0;if(y<radius||y>=r.h-radius){const float dy=y<radius?radius-y-0.5f:y-(r.h-radius)+0.5f;inset=int(ceilf(radius-sqrtf(radius*radius-dy*dy)));}
   const unsigned light=(r.h-1-y)*(active?38:18)/std::max(1,r.h-1);screen.drawFastHLine(r.x+inset,r.y+y,r.w-2*inset,blend(base,edge,light));}
  screen.drawRoundRect(r.x,r.y,r.w,r.h,radius,blend(base,edge,active?55:25));
  screen.drawRoundRect(r.x+1,r.y+1,r.w-2,r.h-2,std::max(1,radius-1),blend(base,edge,active?120:65));
  screen.drawRoundRect(r.x+2,r.y+2,r.w-4,r.h-4,std::max(1,radius-2),blend(base,edge,active?220:100));
 }
 void button(Action a,Rect r,const char* label,bool chosen=false,bool danger=false,bool enabled=true) {
  const auto base=pressed_==a?rgb(23,73,109):(chosen?(danger?rgb(81,25,49):rgb(8,76,118)):CARD),fg=enabled?(danger?RED:TEXT):MUTED;
  glowPanel(r,base,danger?RED:(chosen||pressed_==a?ACCENT:rgb(59,130,188)),chosen||pressed_==a,8);const auto g=glyph(a);
  const bool only_icon=a==Back||a==MediaPrev||a==MediaNext||a==MediaRewind||a==OpenStorage||a==FilePrevPage||a==FileNextPage;
  if(only_icon)icon(g,r.x+r.w/2-9,r.y+r.h/2-9,fg);
  else {const bool with_icon=g!=Glyph::None&&r.w>=60;if(with_icon)icon(g,r.x+(r.w<75?3:8),r.y+r.h/2-9,fg);text(label,r.x+r.w/2+(with_icon?9:0),r.y+r.h/2,fg,a==TogglePlay||(r.w<75&&strlen(label)>4),MC_DATUM);}
  if(enabled)add(a,(a==Back&&r.y<=2)?Rect{0,0,64,32}:r);
 }

 void header(const char* title,bool back=true) {
  for(int y=0;y<24;++y)screen.drawFastHLine(0,y,320,blend(BG,rgb(17,49,89),(23-y)*4));screen.drawFastHLine(8,23,304,LINE);screen.drawFastHLine(112,23,96,blend(LINE,ACCENT,95));
  if(back){button(Back,{4,1,54,26},"<");text(title,65,2);}else text(title,9,2);
 }
 void nav(Page selected) {
  screen.fillRect(0,215,320,25,BG);screen.drawFastHLine(0,215,320,LINE);
  const Action actions[]={Live,Files,Settings};const char* labels[]={"CAMERA","FILES","SETTINGS"};
  const int active=selected==Page::Live?0:(selected==Page::Files?1:2);
  for(int i=0;i<3;++i){if(i==active)glowPanel({i*107+4,217,99,22},PANEL,ACCENT,true,10);icon(glyph(actions[i]),i*107+13,218,i==active?ACCENT:MUTED);text(labels[i],i*107+66,228,i==active?ACCENT:MUTED,true,MC_DATUM);add(actions[i],{i*107,216,i==2?106:107,24});}
 }
 void notice(const char* s){snprintf(toast_,sizeof(toast_),"%s",s);toast_until_=millis()+1800;redraw_=true;}
 Rect toastRect()const{return page_==Page::Live?Rect{8,177,236,27}:Rect{176,1,140,21};}
 void toast(){if(!toast_[0])return;if(int32_t(toast_until_-millis())<=0){toast_[0]=0;redraw_=true;return;}
  const auto r=toastRect();screen.fillRoundRect(r.x,r.y,r.w,r.h,6,PANEL);char label[80];snprintf(label,sizeof(label),page_==Page::Live?"%.37s":"%.21s",toast_);text(label,r.x+r.w/2,r.y+r.h/2,TEXT,true,MC_DATUM);
 }

 void closeMedia(){mp4_player.close();raw_file_.close();raw_count_=raw_index_=0;raw_playing_=false;media_=Media::None;if(raw_storage_held_){raw_storage_held_=false;camera_storage.release();}}
 void stopRecord(){if(video_recorder.busy()){video_recorder.requestStop();mlx_cap.recording=false;notice("Saving video in the background");}}

 void listFiles(){
  closeMedia();files_.clear();file_page_=0;selecting_=false;
  if(video_recorder.busy()){files_loading_=true;return;}files_loading_=false;
  if(!sdcard_mount()){notice("Insert a FAT32 MicroSD card");return;}
  camera_storage.refreshSpace();GetFileList(CAMERA_STORAGE,"/MLX90640",0,files_);sdcard_size(&sd_total_mb_,&sd_free_mb_);
  std::sort(files_.begin(),files_.end(),[](const FileInfo_t& a,const FileInfo_t& b){return a.path>b.path;});
 }
 void navigate(Page next,bool back=false,bool animate=true) {
  if(next==page_){redraw_=true;return;}
  touch_test_mode=next==Page::TouchTest;info_gesture_.reset();
  if(video_recorder.busy()&&next!=Page::Live)stopRecord();
  if(next==Page::Files&&page_!=Page::Preview&&page_!=Page::Player&&page_!=Page::Confirm)listFiles();
  if(next==Page::Grid||next==Page::Image||next==Page::Range||next==Page::Capture||next==Page::Units){draft_=board_current_settings();}
  if(next==Page::Touch){touch_x_=tch_cnf.offset[0];touch_y_=tch_cnf.offset[1];}
  if(next==Page::Live)board_media_focus=false;
  pressed_=None;touch_clear();
  if(slide_old_&&slide_new_&&animate){memcpy(slide_old_,screen.getBuffer(),320*240*2);page_=next;draw();memcpy(slide_new_,screen.getBuffer(),320*240*2);animating_=true;anim_step_=0;anim_tick_=0;backwards_=back;}
  else{page_=next;redraw_=true;}
 }
 void animate(){
  static const unsigned offset[]={48,160,272,320};
  const uint32_t now=millis();if(anim_tick_&&now-anim_tick_<34)return;anim_tick_=now;
  const unsigned n=offset[anim_step_++];auto* dst=static_cast<uint16_t*>(screen.getBuffer());
  for(int y=0;y<240;++y){auto* d=dst+y*320;auto* a=slide_old_+y*320;auto* b=slide_new_+y*320;
   if(!backwards_){memcpy(d,a+n,(320-n)*2);memcpy(d+320-n,b,n*2);}
   else{memcpy(d,b+320-n,n*2);memcpy(d+n,a,(320-n)*2);}}
  screen_dirty=true;if(anim_step_==4){animating_=false;redraw_=true;}
 }
 void applyDraft(){
  if(!camera::valid_settings(draft_)){notice("Invalid setting");return;}
  mlx_cnf.interpolation=draft_.interpolation;mlx_cnf.box_size=draft_.box;mlx_cnf.refresh_rate=camera::sensor_refresh_for_fps(draft_.fps);
  mlx_cnf.color_scheme=draft_.color;mlx_cnf.marker_mode=draft_.marker;mlx_cnf.range_auto=draft_.automatic;
  mlx_cnf.range_min=draft_.minimum;mlx_cnf.range_max=draft_.maximum;mlx_cap.capture_mode=draft_.capture;
  te_pick.x=draft_.pick_x;te_pick.y=draft_.pick_y;mlx_cnf.init();board_temperature_unit=draft_.unit;board_select_fps(draft_.fps);
  const bool ok=board_settings_save();navigate(Page::Settings,true);notice(ok?"Settings saved":"Cannot save settings");
 }
 void scaleChange(bool interpolation,int direction){
  const int choices[]={1,2,4,6,8};const int boxes[]={1,2,4,8};
  auto& v=interpolation?draft_.interpolation:draft_.box;const int* a=interpolation?choices:boxes;const int count=interpolation?5:4;
  int i=0;while(i<count-1&&a[i]!=v)++i;i=std::max(0,std::min(count-1,i+direction));v=a[i];
  if(draft_.interpolation*draft_.box>8){if(interpolation){while(draft_.interpolation*draft_.box>8)draft_.box/=2;}else{while(draft_.interpolation*draft_.box>8){int k=0;while(k<4&&choices[k]!=draft_.interpolation)++k;draft_.interpolation=choices[std::max(0,k-1)];}}}
 }
 void heat(const float* values,Rect r,int interpolation,int palette,float low,float high) {
  const float gain=(N_HEATMAP-1)/std::max(.01f,high-low);
  for(int i=0;i<768;++i)levels_[i]=int32_t(std::max(-4096.f,std::min(4096.f,(values[i]-low)*gain))*256.f);
  uint16_t row[320];uint8_t source_x[320],next_x[320],fraction_x[320];
  for(int x=0;x<r.w;++x){const int q=((x*(32*interpolation)/r.w)<<8)/interpolation;source_x[x]=std::min(31,q>>8);next_x[x]=std::min(31,int(source_x[x])+1);fraction_x[x]=q&255;}
  for(int y=0;y<r.h;++y){const int qy=((y*(24*interpolation)/r.h)<<8)/interpolation;const int sy=std::min(23,qy>>8),ny=std::min(23,sy+1),fy=qy&255;
   for(int x=0;x<r.w;++x){const int sx=source_x[x],nx=next_x[x],fx=fraction_x[x];
    const int a=levels_[sy*32+sx]+((levels_[ny*32+sx]-levels_[sy*32+sx])*fy>>8);
    const int b=levels_[sy*32+nx]+((levels_[ny*32+nx]-levels_[sy*32+nx])*fy>>8);
    const int k=constrain((a+((b-a)*fx>>8))>>8,0,N_HEATMAP-1);const uint16_t c=heatmap[palette][k];
    row[BOARD_SENSOR_OUTWARD?r.w-1-x:x]=uint16_t((c<<8)|(c>>8));}
   auto* dst=static_cast<uint16_t*>(screen.getBuffer())+(r.y+y)*320+r.x;memcpy(dst,row,r.w*2);
  }
 }
 void marker(const Temperature_t& t,bool spot){
  const int px=heat_.x+int((BOARD_SENSOR_OUTWARD?31-t.x:t.x)*heat_.w/32),py=heat_.y+int(t.y*heat_.h/24);
  const auto color=spot?TEXT:(&t==&te_max?AMBER:ACCENT);screen.drawCircle(px,py,5,BG);screen.drawCircle(px,py,4,color);screen.drawFastHLine(px-6,py,13,color);screen.drawFastVLine(px,py-6,13,color);
  char b[20];snprintf(b,sizeof(b),"%.1f",temperature(t.v));screen.setFont(modern_type::ready?static_cast<const lgfx::IFont*>(&modern_type::caption):static_cast<const lgfx::IFont*>(&fonts::Font0));
  const int w=screen.textWidth(b)+12,h=18;const int candidates[6][2]={{px-w/2,py+9},{px-w/2,py-h-9},{px+10,py-h/2},{px-w-10,py-h/2},{px-w/2,py+h+12},{px-w/2,py-h*2-12}};Rect label{};
  for(unsigned k=0;k<6;++k){label={constrain(candidates[k][0],5,243-w),constrain(candidates[k][1],25,203-h),w,h};bool collision=false;
   for(unsigned i=0;i<measurement_label_count_;++i){const auto r=measurement_labels_[i];if(label.x<r.x+r.w+2&&label.x+label.w+2>r.x&&label.y<r.y+r.h+2&&label.y+label.h+2>r.y)collision=true;}if(!collision)break;}
  if(measurement_label_count_<3)measurement_labels_[measurement_label_count_++]=label;
  screen.fillRoundRect(label.x,label.y,label.w,label.h,5,BG);screen.drawRoundRect(label.x,label.y,label.w,label.h,5,blend(BG,color,175));text(b,label.x+label.w/2,label.y+label.h/2,color,true,MC_DATUM);
 }
 void liveImage(){
  const int product=mlx_cnf.interpolation*mlx_cnf.box_size,w=product==8?240:32*product,h=product==8?180:24*product;
  heat_={4+(240-w)/2,24+(180-h)/2,w,h};measurement_label_count_=0;if(w<240||h<180)screen.fillRect(4,24,240,180,BG);
  if(valid_frame_){heat(frame_,heat_,mlx_cnf.interpolation,mlx_cnf.color_scheme,mlx_cnf.range_min,mlx_cnf.range_max);
   screen.setClipRect(4,24,240,180);if(mlx_cnf.marker_mode&1){marker(te_min,false);marker(te_max,false);}if(mlx_cnf.marker_mode&2)marker(te_pick,true);screen.clearClipRect();
  }else{text(board_sensor_error.load()?"Sensor unavailable":"Waiting for MLX90640",124,102,board_sensor_error.load()?RED:MUTED,false,MC_DATUM);text("3V3 / SDA16 / SCL15",124,123,MUTED,true,MC_DATUM);}
 }
 bool hud(bool force=false){
  bool painted=false;char b[40];const auto state=video_recorder.state();const bool rec=video_recorder.active(),waiting=state==VideoRecorder::State::Starting||state==VideoRecorder::State::Stopping;
  const uint16_t cap_bg=pressed_==CaptureNow?LINE:(waiting?rgb(76,56,20):(rec?rgb(83,23,47):rgb(9,71,110))),ink=TEXT;
  auto changed=[&](int i,const char* value){if(force||strcmp(hud_labels_[i],value)){snprintf(hud_labels_[i],sizeof(hud_labels_[i]),"%s",value);painted=true;return true;}return false;};
  if(force){
   screen.fillRect(0,0,320,22,BG);icon(Glyph::Camera,8,1,ACCENT);text("THERMAL",33,2);
   screen.fillRect(248,24,72,188,BG);glowPanel({250,26,66,42},PANEL,ACCENT,false,8);glowPanel({250,70,66,42},PANEL,AMBER,false,8);glowPanel({250,114,66,34},PANEL,ACCENT,false,8);icon(Glyph::Min,250,27,ACCENT);text("MIN",272,29,MUTED,true);icon(Glyph::Max,250,71,AMBER);text("MAX",272,73,MUTED,true);text(unitSuffix(),167,5,ACCENT,true,TR_DATUM);
   hud_state_=-1;hud_mode_=-1;hud_pulse_=-99;hud_spinner_=-1;hud_low_=-999;hud_high_=-999;hud_palette_=-1;hud_spot_=-1;painted=true;
  }
  if(perf_display_fps.load()>0.1f)snprintf(b,sizeof(b),"%.1f FPS",perf_display_fps.load());else snprintf(b,sizeof(b),"FPS --");if(changed(0,b)){screen.fillRect(200,3,57,17,BG);text(b,201,7,MUTED,true);}
  if(board_battery_valid)snprintf(b,sizeof(b),"%u%%",board_battery_level);else snprintf(b,sizeof(b),"--%%");
  if(changed(1,b)){screen.fillRect(265,3,53,17,BG);text(b,286,7,board_battery_level<=20?AMBER:ACCENT,true,MC_DATUM);screen.drawRoundRect(304,5,11,9,2,MUTED);if(board_battery_valid)screen.fillRect(306,7,7*board_battery_level/100,5,ACCENT);}
  const int pulse=(rec||waiting)?int((millis()/60)%20):-1;
  if(force||pulse!=hud_pulse_){hud_pulse_=pulse;screen.fillRect(183,3,16,16,BG);if(pulse>=0){screen.drawCircle(190,10,5,rgb(70,20,35));screen.fillCircle(190,10,3,rgb(175+8*(pulse<10?pulse:19-pulse),55,85));}else screen.fillCircle(190,10,3,ACCENT);painted=true;}
  snprintf(b,sizeof(b),"%.1f",temperature(valid_frame_?te_min.v:0));if(changed(2,b)){screen.fillRect(254,42,59,23,PANEL);number(b,254,42,ACCENT);}
  snprintf(b,sizeof(b),"%.1f",temperature(valid_frame_?te_max.v:0));if(changed(3,b)){screen.fillRect(254,86,59,23,PANEL);number(b,254,86,AMBER);}
  const int spot=(mlx_cnf.marker_mode&2)!=0;if(force||hud_spot_!=spot){hud_spot_=spot;glowPanel({250,114,66,34},PANEL,ACCENT,false,8);text(spot?"SPOT":"SENSOR",254,116,MUTED,true);hud_labels_[4][0]=0;painted=true;}
  snprintf(b,sizeof(b),"%.1f %s",temperature(spot?te_pick.v:board_sensor_ambient.load()),unitSuffix());if(changed(4,b)){screen.fillRect(254,130,59,14,PANEL);text(b,254,131,TEXT,true);}
  const bool cap_changed=force||hud_state_!=int(state)||hud_mode_!=mlx_cap.capture_mode||hud_pressed_!=(pressed_==CaptureNow);
  if(cap_changed){hud_state_=int(state);hud_mode_=mlx_cap.capture_mode;hud_pressed_=pressed_==CaptureNow;glowPanel({250,153,66,52},cap_bg,waiting?AMBER:(rec?RED:ACCENT),true,10);
   if(!waiting)icon(rec?Glyph::Stop:(mlx_cap.capture_mode?Glyph::Video:Glyph::Camera),271,153,ink,0);
   text(state==VideoRecorder::State::Starting?"WAIT":(state==VideoRecorder::State::Stopping?"SAVE":(rec?"STOP":(mlx_cap.capture_mode?"REC":"PHOTO"))),283,185,ink,false,MC_DATUM);
   hud_labels_[5][0]=0;hud_spinner_=-1;painted=true;
  }
  if(waiting){const int phase=(millis()/80)%8;if(phase!=hud_spinner_){hud_spinner_=phase;screen.fillRect(271,154,24,22,cap_bg);static const int dx[]={0,5,7,5,0,-5,-7,-5},dy[]={-7,-5,0,5,7,5,0,-5};for(int i=0;i<8;++i)screen.fillCircle(283+dx[i],166+dy[i],i==phase?2:1,ink);painted=true;}}
  if(rec){const uint32_t secs=video_recorder.elapsedMs()/1000;snprintf(b,sizeof(b),"%02u:%02u",unsigned(secs/60),unsigned(secs%60));}else snprintf(b,sizeof(b),waiting?"MicroSD":(mlx_cap.capture_mode?"MP4":"BMP"));
  if(changed(5,b)){screen.fillRect(254,193,58,9,cap_bg);text(b,283,198,ink,true,MC_DATUM);}
  if(force||hud_low_!=mlx_cnf.range_min||hud_high_!=mlx_cnf.range_max||hud_palette_!=mlx_cnf.color_scheme){hud_low_=mlx_cnf.range_min;hud_high_=mlx_cnf.range_max;hud_palette_=mlx_cnf.color_scheme;
   screen.fillRect(4,205,240,10,BG);snprintf(b,sizeof(b),"%.0f",temperature(mlx_cnf.range_min));text(b,4,207,MUTED,true);snprintf(b,sizeof(b),"%.0f %s",temperature(mlx_cnf.range_max),unitSuffix());text(b,244,207,MUTED,true,TR_DATUM);
   for(int x=0;x<144;++x)screen.drawFastVLine(50+x,208,4,heatmap[mlx_cnf.color_scheme][x*(N_HEATMAP-1)/143]);painted=true;
  }
  if(painted)screen_dirty=true;return painted;
 }

 bool isSlider(Action a)const{return a>=InterpSlider&&a<=FpsSlider;}
 void slider(Action a,Rect r,int value,int lo,int hi,const int* ticks,unsigned n,bool enabled=true){
  const int start=r.x+9,end=r.x+r.w-9,cy=r.y+r.h/2,pos=start+(end-start)*(value-lo)/(hi-lo);
  screen.fillRoundRect(start,cy-3,end-start,6,3,LINE);if(enabled)screen.fillRoundRect(start,cy-3,std::max(1,pos-start),6,3,ACCENT);
  for(unsigned i=0;i<n;++i){const int x=start+(end-start)*(ticks[i]-lo)/(hi-lo);screen.drawFastVLine(x,cy+8,3,MUTED);char b[16];if(a==LowSlider||a==HighSlider)snprintf(b,sizeof(b),"%.0f",camera::display_temperature(ticks[i],draft_.unit));else snprintf(b,sizeof(b),"%d",ticks[i]);text(b,x,cy+12,MUTED,true,TC_DATUM);}
  const int radius=pressed_==a?10:8;screen.fillCircle(pos,cy+1,radius+1,BG);screen.fillCircle(pos,cy,radius,enabled?ACCENT:MUTED);screen.fillCircle(pos,cy,3,BG);
  if(enabled)add(a,{r.x-7,r.y-8,r.w+14,r.h+16});
 }
 bool setSlider(Action a,uint16_t x){
  Rect r{};bool found=false;for(unsigned i=0;i<hit_count_;++i)if(hits_[i].action==a){r=hits_[i].rect;found=true;break;}if(!found)return false;
  r={r.x+7,r.y+8,r.w-14,r.h-16};
  const int width=r.w-18,p=constrain(int(x)-r.x-9,0,width);int value=0;
  const int scales[]={1,2,4,6,8},boxes[]={1,2,4,8};
  if(a==FpsSlider){const int raw=2+(p*6+width/2)/width;const int rates[]={2,4,6,8};value=rates[0];for(int v:rates)if(abs(v-raw)<abs(value-raw))value=v;if(draft_.fps==value)return false;draft_.fps=value;draft_.refresh=camera::sensor_refresh_for_fps(value);return true;}
  if(a==InterpSlider||a==BoxSlider){const int raw=1+(p*7+width/2)/width;const int* values=a==InterpSlider?scales:boxes;const int n=a==InterpSlider?5:4;value=values[0];for(int i=1;i<n;++i)if(abs(values[i]-raw)<abs(value-raw))value=values[i];
   if(a==InterpSlider){if(draft_.interpolation==value)return false;draft_.interpolation=value;while(draft_.interpolation*draft_.box>8)draft_.box/=2;}
   else{if(draft_.box==value)return false;draft_.box=value;while(draft_.interpolation*draft_.box>8){int i=0;while(i<4&&scales[i]!=draft_.interpolation)++i;draft_.interpolation=scales[std::max(0,i-1)];}}return true;}
  if(a==LowSlider||a==HighSlider){value=-40+(p*340+width/2)/width;if(a==LowSlider){value=std::min(value,int(draft_.maximum)-1);if(draft_.minimum==value)return false;draft_.minimum=value;}else{value=std::max(value,int(draft_.minimum)+1);if(draft_.maximum==value)return false;draft_.maximum=value;}return true;}
  value=-10+(p*20+width/2)/width;auto& target=a==OffsetXSlider?touch_x_:touch_y_;if(target==value)return false;target=value;return true;
 }
 void settingRow(Action a,int y,const char* title,const char* detail){glowPanel({8,y,304,32},pressed_==a?LINE:PANEL,ACCENT,pressed_==a,8);icon(glyph(a),14,y+4,ACCENT,0);text(title,44,y+3);text(detail,303,y+16,MUTED,true,TR_DATUM);add(a,{8,y,304,32});}
 void stepper(const char* title,const char* value,int y,Action minus,Action plus,bool enabled=true){
  text(title,13,y+2,MUTED);text(value,151,y+17,TEXT,false,MC_DATUM);button(minus,{206,y,45,32},"-",false,false,enabled);button(plus,{260,y,45,32},"+",false,false,enabled);
 }
 void applyFooter(){button(Apply,{12,181,296,30},"APPLY & SAVE",true);nav(Page::Settings);}
 void drawSettings(){
  header("SETTINGS",false);const Action actions[]={OpenGrid,OpenImage,OpenRange,OpenUnits,OpenCapture,OpenMore};
  const char* labels[]={"Resolution","Image","Temperature","Units","Capture","Device"};
  const char* detail[]={"Detail & frame rate","Palette & markers","Range & auto","Celsius / F / K","Photo & video","Touch & system"};
  for(unsigned i=0;i<6;++i){const int x=i%2?164:8,y=31+(i/2)*59;glowPanel({x,y,148,51},pressed_==actions[i]?LINE:PANEL,ACCENT,pressed_==actions[i],10);icon(glyph(actions[i]),x+9,y+12,ACCENT,0);text(labels[i],x+41,y+7);text(detail[i],x+41,y+29,MUTED,true);add(actions[i],{x,y,148,51});}nav(Page::Settings);
 }
 void drawUnits(){
  header("TEMPERATURE UNITS");const Action actions[]={UnitC,UnitF,UnitK};const char* names[]={"Celsius","Fahrenheit","Kelvin"};const char* symbols[]={"°C","°F","K"};
  for(int i=0;i<3;++i){const int y=33+i*47;const bool chosen=draft_.unit==i;glowPanel({12,y,296,41},chosen?CARD:PANEL,ACCENT,chosen||pressed_==actions[i],10);text(symbols[i],33,y+12,chosen?ACCENT:TEXT);text(names[i],80,y+11);if(chosen)icon(Glyph::Check,273,y+11,ACCENT);add(actions[i],{12,y,296,41});}
  applyFooter();
 }
 void drawGrid(){
  header("RESOLUTION & FPS");const int scales[]={1,2,4,6,8},boxes[]={1,2,4,8},rates[]={2,4,6,8};char b[24];
  text("Interpolation",13,29,MUTED);snprintf(b,sizeof(b),"%ux",draft_.interpolation);text(b,305,29,ACCENT,false,TR_DATUM);
  button(InterpPrev,{8,45,44,34},"-");slider(InterpSlider,{60,50,200,24},draft_.interpolation,1,8,scales,5);button(InterpNext,{268,45,44,34},"+");
  text("Pixel size",13,89,MUTED);snprintf(b,sizeof(b),"%ux",draft_.box);text(b,305,89,ACCENT,false,TR_DATUM);
  button(BoxPrev,{8,105,44,34},"-");slider(BoxSlider,{60,110,200,24},draft_.box,1,8,boxes,4);button(BoxNext,{268,105,44,34},"+");
  text("Frame rate",13,149,MUTED);snprintf(b,sizeof(b),"%u kHz",unsigned(camera::sensor_clock_for_fps(draft_.fps)/1000));text(b,215,155,MUTED,true,TR_DATUM);snprintf(b,sizeof(b),"%u FPS",draft_.fps);text(b,305,149,ACCENT,false,TR_DATUM);
  button(FpsPrev,{8,165,44,34},"-");slider(FpsSlider,{60,170,200,24},draft_.fps,2,8,rates,4);button(FpsNext,{268,165,44,34},"+");
  button(Apply,{12,209,296,30},"APPLY & SAVE",true);
 }

 void drawImage(){header("IMAGE");button(Palette0,{12,34,143,37},"RAINBOW",draft_.color==0);button(Palette1,{165,34,143,37},"INFERNO",draft_.color==1);
  for(int x=0;x<288;++x)screen.drawFastVLine(16+x,78,10,heatmap[draft_.color][x*(N_HEATMAP-1)/287]);
  button(Markers,{12,99,296,32},draft_.marker&1?"MIN / MAX MARKERS   ON":"MIN / MAX MARKERS   OFF",draft_.marker&1);
  button(Spot,{12,140,296,32},draft_.marker&2?"SPOT MEASUREMENT   ON":"SPOT MEASUREMENT   OFF",draft_.marker&2);applyFooter();
 }
 void drawRange(){
  header("TEMPERATURE RANGE");button(Auto,{12,35,296,24},draft_.automatic?"AUTO RANGE":"MANUAL RANGE",draft_.automatic);const int ticks[]={-40,130,300};char b[16];
  text("Low limit",13,63,MUTED);snprintf(b,sizeof(b),"%.1f %s",camera::display_temperature(draft_.minimum,draft_.unit),draftSuffix());text(b,305,63,ACCENT,false,TR_DATUM);
  button(MinMinus,{8,81,44,34},"-",false,false,!draft_.automatic);slider(LowSlider,{60,85,200,26},draft_.minimum,-40,300,ticks,3,!draft_.automatic);button(MinPlus,{268,81,44,34},"+",false,false,!draft_.automatic);
  text("High limit",13,121,MUTED);snprintf(b,sizeof(b),"%.1f %s",camera::display_temperature(draft_.maximum,draft_.unit),draftSuffix());text(b,305,121,AMBER,false,TR_DATUM);
  button(MaxMinus,{8,139,44,34},"-",false,false,!draft_.automatic);slider(HighSlider,{60,143,200,26},draft_.maximum,-40,300,ticks,3,!draft_.automatic);button(MaxPlus,{268,139,44,34},"+",false,false,!draft_.automatic);applyFooter();
 }

 void drawCapture(){header("CAPTURE MODE");
  for(int i=0;i<2;++i){const int x=i?165:12;const bool chosen=draft_.capture==i;screen.fillRoundRect(x,39,143,79,8,chosen?ACCENT:CARD);icon(i?Glyph::Video:Glyph::Camera,x+54,47,chosen?BG:TEXT,2);text(i?"VIDEO":"PHOTO",x+71,100,chosen?BG:TEXT,false,MC_DATUM);add(i?Video:Photo,{x,39,143,79});}
  text(draft_.capture?"MP4 / full screen / temperature overlay":"BMP / full screen / lossless image",160,143,MUTED,true,MC_DATUM);applyFooter();
 }

 void drawMore(){header("DEVICE");settingRow(OpenTouch,37,"Touch alignment","adjust offset / reset");settingRow(OpenInfo,84,"Device information","battery / memory / performance");settingRow(Reset,131,"Reset application settings","saved defaults; files stay on SD");nav(Page::Settings);}
 void drawTouch(){
  header("TOUCH ALIGNMENT");const int ticks[]={-10,0,10};char b[16];text("Horizontal",13,31,MUTED);snprintf(b,sizeof(b),"%+d px",touch_x_);text(b,305,31,ACCENT,false,TR_DATUM);
  button(TouchXMinus,{8,49,44,34},"-");slider(OffsetXSlider,{60,53,200,26},touch_x_,-10,10,ticks,3);button(TouchXPlus,{268,49,44,34},"+");
  text("Vertical",13,92,MUTED);snprintf(b,sizeof(b),"%+d px",touch_y_);text(b,305,92,ACCENT,false,TR_DATUM);
  button(TouchYMinus,{8,110,44,34},"-");slider(OffsetYSlider,{60,114,200,26},touch_y_,-10,10,ticks,3);button(TouchYPlus,{268,110,44,34},"+");
  button(TouchReset,{12,151,143,24},"RESET OFFSET");button(TouchTestStart,{165,151,143,24},"CALIBRATE");applyFooter();
 }

 void drawInfo(){header("DEVICE INFO");char b[80];text("LCDWIKI ES3C28P / ESP32-S3",12,35);text("THERMAL UI 3  /  MLX90640 32 x 24",12,63,MUTED,true);
  snprintf(b,sizeof(b),"Battery: %.3f V  /  %u%% estimated",board_battery_mv/1000,board_battery_level);text(b,12,85,MUTED,true);
  snprintf(b,sizeof(b),"PSRAM: %u KB  /  free heap: %u KB",ESP.getPsramSize()/1024,ESP.getFreeHeap()/1024);text(b,12,104,MUTED,true);
  snprintf(b,sizeof(b),"Sensor bus: %u kHz  /  I2C errors: %u",board_sensor_hz.load()/1000,board_i2c_errors.load());text(b,12,123,MUTED,true);
  text("MP4: Motion JPEG, complete thermal frames",12,145,MUTED,true);text("Based on embedded-kiddie / MLX90640 (MIT)",12,165,MUTED,true);text("Melexis + Adafruit driver",12,183,MUTED,true);nav(Page::Settings);
 }
 unsigned selectedCount()const{unsigned n=0;for(const auto& f:files_)if(f.isSelected)++n;return n;}
 const char* base(const char* p)const{const char* q=strrchr(p,'/');return q?q+1:p;}
 void drawFiles(){header("FILES",false);char b[40];snprintf(b,sizeof(b),"%u files",unsigned(files_.size()));if(!selecting_)text(b,256,7,MUTED,true,TR_DATUM);button(OpenStorage,{272,1,44,27},"");if(sd_free_mb_>=1024)snprintf(b,sizeof(b),"Free %.1fG",sd_free_mb_/1024.0);else snprintf(b,sizeof(b),"Free %uM",sd_free_mb_);if(selecting_)snprintf(b,sizeof(b),"%u selected",selectedCount());text(b,115,8,MUTED,true);
  for(unsigned row=0;row<4;++row){const unsigned i=file_page_*4+row;const int y=31+row*35;if(i>=files_.size())break;const auto& f=files_[i];const auto action=Action(File0+row);
   screen.fillRoundRect(8,y,304,31,6,pressed_==action?LINE:(f.isSelected?rgb(23,77,78):PANEL));
   if(selecting_){screen.drawRoundRect(16,y+8,14,14,3,f.isSelected?ACCENT:MUTED);if(f.isSelected)screen.fillRoundRect(19,y+11,8,8,2,ACCENT);}
   if(!selecting_){const char* e=strrchr(f.path.c_str(),'.');icon(e&&!strcasecmp(e,".mp4")?Glyph::Video:(e&&!strcasecmp(e,".bmp")?Glyph::Photo:Glyph::Grid),17,y+5,ACCENT);}text(base(f.path.c_str()),42,y+1);snprintf(b,sizeof(b),"%.1f KB",f.size/1024.0f);text(b,302,y+20,MUTED,true,TR_DATUM);add(action,{8,y,304,31});
  }
  if(files_.empty())text(files_loading_?"Finalizing the current video...":"No captures yet",160,100,MUTED,false,MC_DATUM);
  button(FilePrevPage,{8,177,49,31},"<",false,false,file_page_>0);button(FileNextPage,{63,177,49,31},">",false,false,(file_page_+1)*4<files_.size());
  button(Select,{120,177,90,31},selecting_?"DONE":"SELECT",selecting_);
  if(selecting_)button(All,{190,1,56,21},"ALL");button(Delete,{218,177,94,31},"DELETE",false,true,selectedCount()>0&&camera_storage.ready());nav(Page::Files);
 }
 bool readRaw(uint32_t index){
  if(!raw_file_||index>=raw_count_||!raw_file_.seek(index*768*sizeof(float))||raw_file_.read(reinterpret_cast<uint8_t*>(raw_),sizeof(raw_))!=sizeof(raw_))return false;
  for(float v:raw_)if(!std::isfinite(v))return false;raw_index_=index;return true;
 }
 bool openMedia(const char* path){
  closeMedia();if(strlen(path)>=sizeof(media_path_))return false;snprintf(media_path_,sizeof(media_path_),"%s",path);const char* ext=strrchr(path,'.');if(!ext)return false;
  if(!strcasecmp(ext,".mp4")){if(!mp4_player.open(path))return false;media_=Media::Mp4;}
  else if(!strcasecmp(ext,".bmp"))media_=Media::Photo;
  else if(!strcasecmp(ext,".raw")){if(!camera_storage.acquire())return false;raw_storage_held_=true;raw_file_=CAMERA_STORAGE.open(path,FILE_READ);if(!raw_file_){closeMedia();return false;}raw_count_=raw_file_.size()/sizeof(raw_);if(!raw_count_||raw_file_.size()%sizeof(raw_)||!readRaw(0)){closeMedia();return false;}media_=Media::Raw;}
  else return false;
  board_media_focus=true;return true;
 }
 void mediaDraw(bool full){
  const Rect r=full?Rect{0,0,320,240}:Rect{40,26,240,180};
  if(media_==Media::Mp4){if(full)mp4_player.draw(true);else mp4_player.drawRegion(r.x,r.y,r.w,r.h);}
  else if(media_==Media::Photo){StorageUse use;if(use)screen.drawBmpFile(CAMERA_STORAGE,media_path_,r.x,r.y,r.w,r.h,0,0,r.w/320.0f,r.h/240.0f);}
  else if(media_==Media::Raw){
   float lo=mlx_cnf.range_min,hi=mlx_cnf.range_max;if(mlx_cnf.range_auto){auto mm=std::minmax_element(raw_,raw_+768);lo=*mm.first;hi=std::max(lo+1,*mm.second);}
   if(full){ // Keep the 4:3 raw image away from player controls.
    heat(raw_,{0,0,320,240},8,mlx_cnf.color_scheme,lo,hi);
   }else heat(raw_,r,8,mlx_cnf.color_scheme,lo,hi);
  }
 }
 void drawPreview(){header(base(media_path_));mediaDraw(false);
  if(media_==Media::Photo)button(MediaPlay,{8,210,304,28},"VIEW FULL SCREEN",true);
  else {button(MediaRewind,{5,210,55,28},"|<");button(MediaPlay,{66,210,121,28},"PLAY",true);button(MediaPrev,{193,210,57,28},"<");button(MediaNext,{256,210,59,28},">");}
 }
 bool mediaPlaying()const{return media_==Media::Mp4?mp4_player.playing():raw_playing_;}
 void playerOverlay(){
  hit_count_=0;screen.fillRect(0,0,320,24,BG);button(Back,{3,1,54,26},"<");char b[48];
  if(media_==Media::Mp4)snprintf(b,sizeof(b),"%s  %.1f / %.1fs",mediaPlaying()?"PLAY":"PAUSE",mp4_player.currentTime()/1000.f,mp4_player.totalTime()/1000.f);
  else if(media_==Media::Raw)snprintf(b,sizeof(b),"RAW  %u / %u",raw_index_+1,raw_count_);else snprintf(b,sizeof(b),"PHOTO");
  text(b,65,8,MUTED,true);if(media_!=Media::Photo)button(TogglePlay,{256,1,61,21},mediaPlaying()?"PAUSE":"PLAY",mediaPlaying());
  if(media_==Media::Mp4&&mp4_player.totalTime()){screen.fillRect(0,237,320,3,LINE);screen.fillRect(0,237,uint64_t(mp4_player.currentTime())*320/mp4_player.totalTime(),3,ACCENT);}
  screen_dirty=true;
 }
 void drawPlayer(){mediaDraw(true);playerOverlay();}
 void drawTouchTest(){
  char b[80];snprintf(b,sizeof(b),"TOUCH TEST %u / 6",test_point_+1);text(b,160,1,TEXT,true,TC_DATUM);
  text("Hold circle; lift when green",160,15,MUTED,true,TC_DATUM);
  const int x=test_targets_[test_point_][0],y=test_targets_[test_point_][1];
  screen.drawCircle(x,y,13,test_ready_?ACCENT:AMBER);screen.drawCircle(x,y,14,test_ready_?ACCENT:AMBER);
  screen.drawFastHLine(x-9,y,19,test_ready_?ACCENT:AMBER);screen.drawFastVLine(x,y-9,19,test_ready_?ACCENT:AMBER);
  if(test_held_){const uint32_t xy=board_touch_unfiltered_xy.load();screen.fillCircle(xy&65535,(xy>>16)&32767,3,RED);}
  snprintf(b,sizeof(b),"Target %d,%d / offset %+d,%+d",x,y,tch_cnf.offset[0],tch_cnf.offset[1]);text(b,160,228,MUTED,true,TC_DATUM);
 }
 void testControls(const Touch_t& t){
  if(t.event&EVENT_DOWN){test_started_=millis();test_count_=0;test_sum_x_=test_sum_y_=test_ui_x_=test_ui_y_=0;test_min_x_=319;test_max_x_=0;test_min_y_=239;test_max_y_=0;test_held_=true;test_ready_=false;redraw_=true;}
  if(test_held_&&(t.event&(EVENT_DOWN|EVENT_TOUCHED))){
   const uint32_t xy=board_touch_unfiltered_xy.load();if(xy&0x80000000u){const unsigned x=xy&65535,y=(xy>>16)&32767;++test_count_;test_sum_x_+=x;test_sum_y_+=y;test_ui_x_+=t.x;test_ui_y_+=t.y;test_min_x_=std::min(unsigned(test_min_x_),x);test_max_x_=std::max(unsigned(test_max_x_),x);test_min_y_=std::min(unsigned(test_min_y_),y);test_max_y_=std::max(unsigned(test_max_y_),y);}
   if(millis()-test_started_>=350&&test_count_>=4)test_ready_=true;
   if(millis()-test_tick_>=80){test_tick_=millis();redraw_=true;}
  }
  if(t.event&EVENT_UP){test_held_=false;
   if(test_ready_&&test_count_){Serial.printf("TOUCH TEST %u target=%u,%u raw=%.1f,%.1f span=%u,%u UI=%.1f,%.1f offset=%d,%d\n",test_point_+1,test_targets_[test_point_][0],test_targets_[test_point_][1],float(test_sum_x_)/test_count_,float(test_sum_y_)/test_count_,test_max_x_-test_min_x_,test_max_y_-test_min_y_,float(test_ui_x_)/test_count_,float(test_ui_y_)/test_count_,tch_cnf.offset[0],tch_cnf.offset[1]);
    test_samples_[test_point_]={float(test_sum_x_)/test_count_,float(test_sum_y_)/test_count_,float(test_targets_[test_point_][0]),float(test_targets_[test_point_][1])};
    if(++test_point_==6){camera::TouchAffine fit;const bool valid=camera::fit_touch_affine(test_samples_,6,fit);bool saved=false;if(valid){touch_affine=fit;tch_cnf.offset[0]=tch_cnf.offset[1]=0;saved=touch_affine_save()&&touch_save(&tch_cnf);}Serial.printf("TOUCH TEST complete: fit=%d saved=%d X=%.5fx %+.5fy %+.2f Y=%.5fx %+.5fy %+.2f\n",valid,saved,touch_affine.xx,touch_affine.xy,touch_affine.x0,touch_affine.yx,touch_affine.yy,touch_affine.y0);navigate(Page::Touch,true,false);notice(valid?(saved?"Touch calibrated & saved":"Calibration save failed"):"Inconsistent taps; try again");return;}
   }
   test_ready_=false;touch_clear();redraw_=true;
  }
 }
 void showFormatPrompt(bool automatic){
  if(video_recorder.busy()){notice("Finish saving before formatting");return;}closeMedia();automatic_format_prompt_=automatic;format_epoch_=camera_storage.epoch();storage_seen_epoch_=format_epoch_;navigate(Page::FormatConfirm,false,false);
 }
 void drawStorage(){
  header("MEMORY CARD");char b[64];icon(Glyph::Storage,22,42,ACCENT,2);
  const auto state=camera_storage.state();const bool ready=state==camera::StorageState::Ready;
  snprintf(b,sizeof(b),"%u MB card",camera_storage.capacityMB());text(camera_storage.capacityMB()?b:"MicroSD",73,39);
  if(ready)snprintf(b,sizeof(b),"%u MB free / %u MB",camera_storage.freeMB(),camera_storage.totalMB());else snprintf(b,sizeof(b),"%s",state==camera::StorageState::Checking?"Checking card...":(state==camera::StorageState::Ejected?"Safe to remove":(state==camera::StorageState::NeedsFormat?"Format required":(state==camera::StorageState::Absent?"Insert a MicroSD card":"Card read error"))));text(b,73,62,MUTED,true);
  button(FormatCard,{12,97,296,40},"FORMAT CARD",false,true,ready||state==camera::StorageState::NeedsFormat);
  button(EjectCard,{12,145,143,40},"EJECT",false,false,ready||state==camera::StorageState::NeedsFormat);button(RetryCard,{165,145,143,40},"RETRY");nav(Page::Files);
 }
 void drawFormatConfirm(){
  header("FORMAT MEMORY CARD");icon(Glyph::Warning,143,35,AMBER,2);
  text(automatic_format_prompt_?"This card needs formatting":"Erase all files on this card?",160,82,TEXT,false,MC_DATUM);
  text("All photos, videos and other files will be lost.",160,105,MUTED,true,MC_DATUM);char b[48];snprintf(b,sizeof(b),"%u MB  /  format as %s",camera_storage.capacityMB(),camera_storage.capacityMB()>=64?"FAT32":"FAT");text(b,160,123,MUTED,true,MC_DATUM);
  button(FormatCancel,{12,157,143,44},automatic_format_prompt_?"CANCEL / EJECT":"CANCEL");button(FormatYes,{165,157,143,44},"FORMAT",true,true);
 }
 void drawFormatBusy(){header("FORMATTING CARD",false);icon(Glyph::Storage,142,57,ACCENT,2);text("Preparing your memory card...",160,115,TEXT,false,MC_DATUM);text("Keep the card inserted until complete.",160,144,MUTED,true,MC_DATUM);}
 void drawConfirm(){header(reset_confirm_?"RESET SETTINGS":"DELETE FILES");screen.fillRoundRect(16,61,288,120,10,PANEL);char b[64];
  if(reset_confirm_)snprintf(b,sizeof(b),"Restore application defaults?");else snprintf(b,sizeof(b),"Delete %u selected file(s)?",selectedCount());
  text(b,160,80,TEXT,false,TC_DATUM);text(reset_confirm_?"Your captures stay on the SD card.":"This removes the selected captures.",160,109,MUTED,true,TC_DATUM);
  button(ConfirmNo,{30,140,120,30},"CANCEL");button(ConfirmYes,{170,140,120,30},reset_confirm_?"RESET":"DELETE",true,true);
 }
 void draw(){
  hit_count_=0;screen.fillScreen(BG);
  switch(page_){
   case Page::Live:liveImage();hud(true);add(CaptureNow,{250,153,66,52});add(Pick,heat_);nav(Page::Live);break;
   case Page::Settings:drawSettings();break;case Page::Grid:drawGrid();break;case Page::Image:drawImage();break;case Page::Range:drawRange();break;case Page::Capture:drawCapture();break;
   case Page::More:drawMore();break;case Page::Touch:drawTouch();break;case Page::Info:drawInfo();break;case Page::Files:drawFiles();break;case Page::Preview:drawPreview();break;case Page::Player:drawPlayer();break;case Page::Confirm:drawConfirm();break;case Page::TouchTest:drawTouchTest();break;case Page::Units:drawUnits();break;case Page::Storage:drawStorage();break;case Page::FormatConfirm:drawFormatConfirm();break;case Page::FormatBusy:drawFormatBusy();break;case Page::GameLoading:header("",false);icon(Glyph::Play,142,61,ACCENT,2);text("Loading DOOM...",160,126,TEXT,false,MC_DATUM);break;
  }
  toast();redraw_=false;screen_dirty=true;
 }
 bool photoSave(){
  if(!valid_frame_||board_sensor_error.load())return false;
  if(!sdcard_mount())return false;const int no=sdcard_fileno();if(no<=0)return false;
  char path[64];snprintf(path,sizeof(path),"/MLX90640/mlx%04d.bmp",no);
  const auto* pixels=slide_old_?slide_old_:static_cast<const uint16_t*>(screen.getBuffer());
  SdBufferedFile f;if(!f.open(path))return false;
  uint8_t header[54]{};header[0]='B';header[1]='M';
  auto le=[&](int at,uint32_t n){for(int k=0;k<4;++k)header[at+k]=n>>(8*k);};
  le(2,54+320*240*3);le(10,54);le(14,40);le(18,320);le(22,240);header[26]=1;header[28]=24;
  bool ok=f.write(header,sizeof(header))==sizeof(header);uint8_t row[320*3];
  for(int y=239;y>=0&&ok;--y){for(int x=0;x<320;++x){const auto v=pixels[y*320+x];const uint16_t c=(v<<8)|(v>>8);const unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;
    row[x*3]=(b<<3)|(b>>2);row[x*3+1]=(g<<2)|(g>>4);row[x*3+2]=(r<<3)|(r>>2);}
   ok=f.write(row,sizeof(row))==sizeof(row);}
  f.flush();ok=ok&&!f.getWriteError();f.close();if(!ok)CAMERA_STORAGE.remove(path);Serial.printf("BMP %s: %s\n",ok?"saved":"failed",path);return ok;
 }
 void capturePhoto(){
  if(page_!=Page::Live||!valid_frame_){notice("Return to a valid camera image");return;}
  if(video_recorder.busy()){notice("Finish the video before taking a photo");return;}
  toast_[0]=0;draw();if(slide_old_)memcpy(slide_old_,screen.getBuffer(),320*240*2);
  notice("Saving photo...");draw();gfx_flush();const bool ok=photoSave();if(ok)camera_storage.refreshSpace();notice(ok?"Photo saved":"Photo failed - check MicroSD");
 }
 void captureVideo(){
  if(video_recorder.busy()){stopRecord();return;}
  if(page_!=Page::Live||!valid_frame_||board_sensor_error.load()){notice("Return to a valid camera image");return;}
  mlx_cap.capture_mode=1;mlx_cap.recording=video_recorder.begin();record_start_=millis();
  notice(mlx_cap.recording?"Preparing MicroSD...":"Cannot start video - check MicroSD");
 }

 void startPlayback(){
  if(media_==Media::Mp4)mp4_player.play(false);
  else if(media_==Media::Raw){if(raw_index_+1>=raw_count_)readRaw(0);raw_started_=millis()-uint64_t(raw_index_)*1000/14;raw_playing_=true;}
  navigate(Page::Player,false,false);
 }
 void pausePlayback(){mp4_player.pause();raw_playing_=false;}
 void mediaStep(int direction){
  pausePlayback();if(media_==Media::Mp4){const int64_t n=int64_t(mp4_player.frameIndex())+direction;mp4_player.seekFrame(std::max(int64_t(0),std::min(int64_t(mp4_player.frameCount()-1),n)));}
  else if(media_==Media::Raw){const int64_t n=int64_t(raw_index_)+direction;readRaw(std::max(int64_t(0),std::min(int64_t(raw_count_-1),n)));}redraw_=true;
 }
 void back(){
  if(page_==Page::FormatBusy)return;
  if(page_==Page::FormatConfirm){if(automatic_format_prompt_)camera_storage.eject();navigate(automatic_format_prompt_?Page::Files:Page::Storage,true,false);return;}
  if(page_==Page::Storage){navigate(Page::Files,true);return;}
  if(page_==Page::Player){pausePlayback();navigate(Page::Preview,true,false);}
  else if(page_==Page::Preview){closeMedia();if(files_.empty())listFiles();navigate(Page::Files,true);}
  else if(page_==Page::Touch||page_==Page::Info)navigate(Page::More,true);
  else if(page_==Page::More)navigate(Page::Settings,true);
  else if(page_==Page::Confirm)navigate(return_page_,true);
  else navigate(Page::Settings,true);
 }
 void execute(Action a,uint16_t x=0,uint16_t y=0){
  if(a>=File0&&a<=File3){const unsigned i=file_page_*4+(a-File0);if(i<files_.size()){
    if(selecting_)files_[i].isSelected=!files_[i].isSelected;
    else if(openMedia(files_[i].path.c_str()))navigate(Page::Preview);
    else notice("Unsupported or incomplete capture");}redraw_=true;return;}
  switch(a){
   case Live:home();break;case Files:navigate(Page::Files);break;case Settings:navigate(Page::Settings);break;case Back:back();break;
   case OpenUnits:navigate(Page::Units);break;case UnitC:draft_.unit=0;break;case UnitF:draft_.unit=1;break;case UnitK:draft_.unit=2;break;
   case OpenStorage:closeMedia();navigate(Page::Storage);break;case FormatCard:showFormatPrompt(false);break;case FormatCancel:back();break;case FormatYes:if(camera_storage.confirmFormat(format_epoch_))navigate(Page::FormatBusy,false,false);else{navigate(Page::Storage,true,false);notice("Card changed or busy; try again");}break;case EjectCard:closeMedia();camera_storage.eject();notice("Ejecting memory card...");break;case RetryCard:camera_storage.retry();break;
   case OpenGrid:navigate(Page::Grid);break;case OpenImage:navigate(Page::Image);break;case OpenRange:navigate(Page::Range);break;case OpenCapture:navigate(Page::Capture);break;case OpenMore:navigate(Page::More);break;case OpenTouch:navigate(Page::Touch);break;case OpenInfo:navigate(Page::Info);break;
   case Apply:if(page_==Page::Touch){tch_cnf.offset[0]=touch_x_;tch_cnf.offset[1]=touch_y_;const bool ok=touch_save(&tch_cnf);navigate(Page::More,true);notice(ok?"Touch alignment saved":"Cannot save alignment");}else applyDraft();break;
   case FpsPrev:case FpsNext:{const int rates[]={2,4,6,8};int i=0;while(i<3&&rates[i]!=draft_.fps)++i;i=constrain(i+(a==FpsNext?1:-1),0,3);draft_.fps=rates[i];draft_.refresh=camera::sensor_refresh_for_fps(draft_.fps);break;}
   case InterpPrev:scaleChange(true,-1);break;case InterpNext:scaleChange(true,1);break;case BoxPrev:scaleChange(false,-1);break;case BoxNext:scaleChange(false,1);break;
   case Palette0:draft_.color=0;break;case Palette1:draft_.color=1;break;case Markers:draft_.marker^=1;break;case Spot:draft_.marker^=2;break;case Auto:draft_.automatic^=1;if(!draft_.automatic){draft_.minimum=constrain(draft_.minimum,-40,299);draft_.maximum=constrain(draft_.maximum,draft_.minimum+1,300);}break;
   case MinMinus:if(!draft_.automatic)draft_.minimum=std::max(-40,int(draft_.minimum)-1);break;case MinPlus:if(!draft_.automatic)draft_.minimum=std::min(int(draft_.maximum)-1,int(draft_.minimum)+1);break;
   case MaxMinus:if(!draft_.automatic)draft_.maximum=std::max(int(draft_.minimum)+1,int(draft_.maximum)-1);break;case MaxPlus:if(!draft_.automatic)draft_.maximum=std::min(300,int(draft_.maximum)+1);break;
   case Photo:draft_.capture=0;break;case Video:draft_.capture=1;break;
   case TouchTestStart:startTouchTest();break;
   case TouchXMinus:touch_x_=std::max(-10,touch_x_-1);break;case TouchXPlus:touch_x_=std::min(10,touch_x_+1);break;case TouchYMinus:touch_y_=std::max(-10,touch_y_-1);break;case TouchYPlus:touch_y_=std::min(10,touch_y_+1);break;case TouchReset:touch_x_=touch_y_=0;break;
   case CaptureNow:if(mlx_cap.capture_mode)captureVideo();else capturePhoto();break;
   case Pick:if(valid_frame_&&heat_.contains(x,y)){
     te_pick.x=std::min(31,(int(x)-heat_.x)*32/heat_.w);if(BOARD_SENSOR_OUTWARD)te_pick.x=31-te_pick.x;
     te_pick.y=std::min(23,(int(y)-heat_.y)*24/heat_.h);mlx_cnf.marker_mode|=2;lp_pick.y=frame_[te_pick.y*32+te_pick.x];board_settings_save();}break;
   case FilePrevPage:if(file_page_)--file_page_;break;case FileNextPage:if((file_page_+1)*4<files_.size())++file_page_;break;
   case Select:selecting_=!selecting_;if(!selecting_)for(auto& f:files_)f.isSelected=false;break;
   case All:{const bool value=selectedCount()!=files_.size();for(auto& f:files_)f.isSelected=value;break;}
   case Delete:if(selectedCount()){reset_confirm_=false;return_page_=Page::Files;navigate(Page::Confirm);}break;
   case Reset:reset_confirm_=true;return_page_=Page::More;navigate(Page::Confirm);break;
   case ConfirmNo:navigate(return_page_,true);break;
   case ConfirmYes:if(reset_confirm_){draft_=camera::Settings{};applyDraft();}else{
     StorageUse storage_use;if(!storage_use){notice("Card unavailable");break;}unsigned deleted=0,failed=0;for(const auto& f:files_)if(f.isSelected){if(CAMERA_STORAGE.remove(f.path.c_str()))++deleted;else ++failed;}
     listFiles();navigate(Page::Files,true);char b[60];snprintf(b,sizeof(b),"Deleted %u / failed %u",deleted,failed);notice(b);}break;
   case MediaPlay:startPlayback();break;
   case MediaRewind:pausePlayback();if(media_==Media::Mp4)mp4_player.seekFrame(0);else if(media_==Media::Raw)readRaw(0);break;
   case MediaPrev:mediaStep(-1);break;case MediaNext:mediaStep(1);break;
   case TogglePlay:if(mediaPlaying())pausePlayback();else{if(media_==Media::Mp4)mp4_player.play(false);else if(media_==Media::Raw){if(raw_index_+1>=raw_count_)readRaw(0);raw_started_=millis()-uint64_t(raw_index_)*1000/14;raw_playing_=true;}}break;
   default:break;
  }
  redraw_=true;
 }
 bool actsOnPress(Action a)const{return a==Back||(a>=InterpPrev&&a<=BoxNext)||(a>=MinMinus&&a<=MaxPlus)||(a>=TouchXMinus&&a<=TouchYPlus)||a==FpsPrev||a==FpsNext;}
 void controls(){
  Touch_t t{};if(!touch_event(t))return;
  if(page_==Page::TouchTest){testControls(t);return;}
  if(page_==Page::Info&&(t.event&EVENT_UP)){const Action action=hit(t.x,t.y);if(info_gesture_.tap(millis(),t.y>=215||action==Back||action==Live||action==Files||action==Settings)){closeMedia();if(video_recorder.busy())stopRecord();navigate(Page::GameLoading,false,false);board_doom_pending=true;return;}}
  if(board_touch_trace&&(t.event&EVENT_CLICK))Serial.printf("UI touch event=%u xy=%u,%u hit=%u\n",unsigned(t.event),t.x,t.y,unsigned(hit(t.x,t.y)));
  if(t.event&EVENT_DOWN){pressed_=hit(t.x,t.y);press_executed_=false;press_start_=repeat_tick_=millis();if(isSlider(pressed_))setSlider(pressed_,t.x);else if(actsOnPress(pressed_)){press_executed_=true;execute(pressed_,t.x,t.y);}redraw_=true;}
  else if(t.event&EVENT_UP){const auto action=pressed_;pressed_=None;if(isSlider(action))setSlider(action,t.x);else if(!press_executed_&&action!=None&&hit(t.x,t.y)==action)execute(action,t.x,t.y);press_executed_=false;redraw_=true;}
  else if(t.event&EVENT_TOUCHED){if(isSlider(pressed_)){if(setSlider(pressed_,t.x))redraw_=true;}else{const uint32_t now=millis();if(pressed_>=MinMinus&&pressed_<=MaxPlus&&now-press_start_>400&&now-repeat_tick_>100&&hit(t.x,t.y)==pressed_){repeat_tick_=now;execute(pressed_,t.x,t.y);}}}
 }

 void playbackTick(){
  if(page_!=Page::Player)return;
  if(media_==Media::Mp4){const auto before=mp4_player.frameIndex();const bool was=mp4_player.playing();mp4_player.tick();if(before!=mp4_player.frameIndex()||was!=mp4_player.playing())playerOverlay();}
  else if(media_==Media::Raw&&raw_playing_){const uint64_t at=uint64_t(millis()-raw_started_)*14/1000;const uint32_t target=std::min(uint64_t(raw_count_-1),at);if(target!=raw_index_){if(readRaw(target)){mediaDraw(true);playerOverlay();}else raw_playing_=false;}if(at>=raw_count_){raw_playing_=false;playerOverlay();}}
 }
 public:
 void startTouchTest(){test_point_=test_count_=0;test_held_=test_ready_=false;navigate(Page::TouchTest,false,false);Serial.println("TOUCH TEST: hold each target at least 350ms, then release; h cancels.");}
 bool begin(){
  modern_type::begin();
  slide_old_=static_cast<uint16_t*>(heap_caps_malloc(320*240*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));slide_new_=static_cast<uint16_t*>(heap_caps_malloc(320*240*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  board_battery_update();touch_clear();board_media_focus=false;page_=Page::Live;draw();return slide_old_&&slide_new_;
 }
 bool wantsLive()const{return page_==Page::Live&&!animating_;}
 void home(){if(video_recorder.busy())stopRecord();closeMedia();navigate(Page::Live,true);}
 bool playPath(const char* path){if(video_recorder.busy()){video_recorder.stop();mlx_cap.recording=false;}if(!openMedia(path)){notice("Cannot open video");return false;}startPlayback();return true;}
 bool update(const float* input){
  bool rendered=false;camera_storage.busy(video_recorder.busy()||bool(raw_file_));
  if((page_==Page::Files||page_==Page::Storage)&&sd_free_mb_!=camera_storage.freeMB()){sd_free_mb_=camera_storage.freeMB();sd_total_mb_=camera_storage.totalMB();redraw_=true;}
  const auto storage_state=camera_storage.state();if(storage_state!=last_storage_state_){last_storage_state_=storage_state;redraw_=true;if(page_==Page::Files&&storage_state==camera::StorageState::Ready)listFiles();}
  if(page_==Page::FormatBusy){const auto result=camera_storage.formatResult();if(result==CameraStorage::FormatResult::Done||result==CameraStorage::FormatResult::Failed||result==CameraStorage::FormatResult::CardChanged){navigate(Page::Storage,false,false);notice(result==CameraStorage::FormatResult::Done?"Card formatted and ready":(result==CameraStorage::FormatResult::CardChanged?"Card changed; confirm again":"Card not formatted; please retry"));}}
  if(storage_state==camera::StorageState::NeedsFormat&&camera_storage.epoch()!=storage_seen_epoch_&&(page_==Page::Live||page_==Page::Files||page_==Page::Storage))showFormatPrompt(true);
  const bool battery_changed=board_media_focus.load()?false:board_battery_update();
  const auto event=video_recorder.pollEvent();
  if(event==VideoRecorder::Event::Started){if(page_!=Page::Live)video_recorder.requestStop();else if(video_recorder.state()==VideoRecorder::State::Recording)notice("Recording MP4");redraw_=true;}
  else if(event==VideoRecorder::Event::Saved){camera_storage.refreshSpace();mlx_cap.recording=false;notice("Video saved");}
  else if(event==VideoRecorder::Event::Cancelled){mlx_cap.recording=false;notice("Recording cancelled");}
  else if(event==VideoRecorder::Event::Failed){mlx_cap.recording=false;notice("Video failed - check MicroSD");}
  if(files_loading_&&!video_recorder.busy()&&page_==Page::Files){listFiles();redraw_=true;}
  if(animating_){animate();return false;}
  controls();if(animating_){animate();return false;}
  if(board_capture_pending.exchange(false))capturePhoto();if(board_video_toggle_pending.exchange(false))captureVideo();
  if(input&&page_==Page::Live){if(!valid_frame_)redraw_=true;memcpy(frame_,input,sizeof(frame_));mlx_cnf.sampling_period=1.0f/board_requested_fps.load();filter_temperature(frame_);valid_frame_=true;
   if(!redraw_){liveImage();hud();toast();screen_dirty=true;}rendered=true;}
  if(page_==Page::Live&&board_sensor_error.load()!=0&&valid_frame_){valid_frame_=false;redraw_=true;}
  if(toast_[0]&&int32_t(toast_until_-millis())<=0){toast_[0]=0;redraw_=true;}
  if(battery_changed&&page_==Page::Live)hud();
  if(page_==Page::Live&&video_recorder.busy()&&millis()-hud_anim_ms_>=80){hud_anim_ms_=millis();hud();}
  playbackTick();if(redraw_)draw();return rendered;
 }

};
inline UI ui;
} // namespace modern
