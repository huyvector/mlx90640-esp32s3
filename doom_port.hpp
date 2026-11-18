#pragma once
#include <LittleFS.h>
extern "C" {
#include "src/doom/doomgeneric.h"
#include "src/doom/doomkeys.h"
void M_SaveDefaults(void);
void Doom_InitInfoTables(void);
void Doom_InitTicData(void);
}
#include "doom_control.hpp"
extern "C" int menuactive;
namespace camera_doom {
inline uint16_t held=0,serial_held=0;inline uint32_t serial_until=0,poll_at=0;inline bool released=false;
struct Key{int pressed;unsigned char key;};inline Key events[64];inline unsigned read_at=0,write_at=0;
inline void emit(bool down,unsigned char key){unsigned next=(write_at+1)%64;if(next!=read_at){events[write_at]={down,key};write_at=next;}}
inline void poll(){
 const uint32_t now=millis();if(now==poll_at)return;poll_at=now;
 const uint32_t raw=board_touch_xy.load();uint16_t x=raw&65535,y=(raw>>16)&32767;const bool pressed=(raw&0x80000000u)!=0;
 camera::align_touch(touch_affine,x,y);x=constrain(int(x)+tch_cnf.offset[0],0,319);y=constrain(int(y)+tch_cnf.offset[1],0,239);
 uint16_t keys=0;
 if(!pressed)released=true;
 if(released&&pressed){
  if(y<29){if(x<64)keys=64;else if(x>263)doom_platform_exit();else if(x>201)keys=128;}
  else if(x<145){const int dx=int(x)-72,dy=int(y)-130;if(dy<-18)keys|=1;if(dy>35)keys|=2;if(dx<-23)keys|=4;if(dx>23)keys|=8;}
  else if(y<88){keys=1024;}
  else if(y<142)keys=32;else keys=16;
 }
 while(Serial.available()){
  const char c=Serial.read();uint16_t key=0;if(c=='h')doom_platform_exit();else if(c=='w')key=1;else if(c=='s')key=2;else if(c=='a')key=4;else if(c=='d')key=8;else if(c=='f')key=16;else if(c==' ')key=32;else if(c=='m'||c=='\n'||c=='\r')key=64;else if(c=='t')key=128;else if(c=='q')key=256;else if(c=='e')key=512;if(key){serial_held=key;serial_until=now+120;}
 }
 if(int32_t(serial_until-now)>0)keys|=serial_held;
 if(menuactive&&(keys&(16|32))){keys&=~uint16_t(16|32);keys|=2048;}
 static const unsigned char codes[]={KEY_UPARROW,KEY_DOWNARROW,KEY_LEFTARROW,KEY_RIGHTARROW,KEY_FIRE,KEY_USE,KEY_ESCAPE,KEY_TAB,',','.','1',KEY_ENTER};
 static unsigned weapon=1;
 for(unsigned i=0;i<12;++i)if((keys^held)&(1u<<i)){const bool down=(keys&(1u<<i))!=0;if(i==10){if(down)weapon=weapon%7+1;emit(down,'0'+weapon);}else emit(down,codes[i]);}
 held=keys;
}
inline void overlay(){
 const uint16_t bg=modern::BG,fg=modern::TEXT,accent=modern::ACCENT;
 screen.setFont(&modern_type::caption);screen.setTextSize(1);screen.setTextDatum(MC_DATUM);screen.setTextColor(fg);
 auto pill=[&](int x,int y,int w,int h,const char* s){screen.fillRoundRect(x,y,w,h,7,bg);screen.drawString(s,x+w/2,y+h/2);};
 pill(3,2,55,24,"MENU");pill(205,2,51,24,"MAP");pill(264,2,53,24,"EXIT");
 screen.drawCircle(71,133,48,accent);screen.fillTriangle(71,91,64,104,78,104,fg);screen.fillTriangle(71,175,64,162,78,162,fg);screen.fillTriangle(29,133,42,126,42,140,fg);screen.fillTriangle(113,133,100,126,100,140,fg);
 pill(250,55,66,27,"WEAPON");pill(250,101,66,34,"USE / OK");pill(250,154,66,40,"FIRE");
}
inline void worker(void*){
 Serial.printf("DOOM starting, free PSRAM=%u\n",ESP.getFreePsram());
 // Internal flash only; this cannot format or depend on the removable SD card.
 const bool saves=LittleFS.begin(true,"/doomfs",5,"spiffs");Serial.printf("DOOM internal saves=%d\n",saves);
 static char a0[]="doom",a1[]="-iwad",a2[]="/rom/doom1.wad",a3[]="-mb",a4[]="4",a5[]="-nosound",a6[]="-config",a7[]="/doomfs/default.cfg",a8[]="-savedir",a9[]="/doomfs/",a10[]="-nogui";
 static char* argv[]={a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10};doomgeneric_Create(11,argv);Serial.println("DOOM ready: touch controls / h returns to camera");
 for(;;){doomgeneric_Tick();delay(1);}
}
}
extern "C" void DG_Init(){Doom_InitInfoTables();Doom_InitTicData();Serial.println("DOOM display initialized");}
extern "C" void DG_DrawFrame(){
 screen.clearClipRect();auto* out=static_cast<uint16_t*>(screen.getBuffer());
 for(unsigned y=0;y<240;++y){const uint32_t* row=DG_ScreenBuffer+(y*200/240)*320;for(unsigned x=0;x<320;++x){const uint32_t c=row[x];const uint16_t v=((c>>8)&0xF800)|((c>>5)&0x07E0)|((c>>3)&0x001F);out[y*320+x]=(v<<8)|(v>>8);}}
 camera_doom::overlay();screen_dirty=true;gfx_flush();
 static uint32_t since=millis(),frames=0;++frames;const auto now=millis();if(now-since>=5000){Serial.printf("DOOM %.1f FPS, free PSRAM=%u\n",frames*1000.0f/(now-since),ESP.getFreePsram());frames=0;since=now;}
}
extern "C" void DG_SleepMs(uint32_t ms){delay(ms?ms:1);}
extern "C" uint32_t DG_GetTicksMs(){return millis();}
extern "C" int DG_GetKey(int* pressed,unsigned char* key){camera_doom::poll();if(camera_doom::read_at==camera_doom::write_at)return 0;const auto e=camera_doom::events[camera_doom::read_at];camera_doom::read_at=(camera_doom::read_at+1)%64;*pressed=e.pressed;*key=e.key;return 1;}
extern "C" void DG_SetWindowTitle(const char*){}
extern "C" void doom_platform_exit(){
 Serial.println("DOOM exiting to camera");M_SaveDefaults();screen.fillScreen(modern::BG);screen.setFont(&modern_type::body);screen.setTextDatum(MC_DATUM);screen.setTextColor(modern::TEXT);screen.drawString("Returning to camera...",160,120);screen_dirty=true;gfx_flush();LittleFS.end();delay(100);ESP.restart();for(;;)delay(100);
}
extern "C" void doom_platform_fatal(const char* message){
 Serial.printf("DOOM error: %s\n",message);screen.fillScreen(modern::BG);screen.setFont(&modern_type::body);screen.setTextColor(modern::RED);screen.setTextDatum(TL_DATUM);screen.drawString("Game could not start",12,55);screen.setFont(&modern_type::caption);screen.setTextColor(modern::TEXT);screen.drawString(String(message).substring(0,48),12,94);screen.drawString("Tap to return to camera",12,145);screen_dirty=true;gfx_flush();delay(500);
 bool released=false;for(;;){const bool p=board_touch_xy.load()&0x80000000u;if(!p)released=true;if(p&&released)ESP.restart();if(Serial.available()&&Serial.read()=='h')ESP.restart();delay(20);}
}
inline bool doom_start(){
 if(board_doom_active.exchange(true))return true;board_media_focus=true;board_acquire_enabled=false;camera_storage.busy(true);touch_clear();
 TaskHandle_t task=nullptr;if(xTaskCreatePinnedToCore(camera_doom::worker,"DOOM",20480,nullptr,1,&task,0)!=pdPASS){board_doom_active=false;board_media_focus=false;camera_storage.busy(false);Serial.println("DOOM task allocation failed");return false;}return true;
}
