#pragma once
#include <Arduino.h>
#include <atomic>
#include <cstring>
#include <FS.h>
#include <vfs_api.h>
#include <sd_defines.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <diskio_impl.h>
#include <diskio_sdmmc.h>
#include <freertos/queue.h>
#include "storage_policy.hpp"
// Own the raw card even when it has no supported filesystem. Mount/probe and
// confirmed formatting run in a worker; ordinary FS paths remain /sdcard/...
class CameraStorage:public fs::FS {
 using State=camera::StorageState;
 public:enum class FormatResult:uint8_t{None,Pending,Done,Failed,CardChanged};
 private:std::atomic<FormatResult> format_result_{FormatResult::None};
 sdmmc_card_t* card_=nullptr;FATFS* fat_=nullptr;BYTE drive_=FF_DRV_NOT_USED;
 char volume_[3]={'0',':',0};bool host_=false,vfs_=false,mounted_=false;
 std::atomic<State> state_{State::Absent};std::atomic<uint32_t> epoch_{1},capacity_mb_{0},total_mb_{0},free_mb_{0};
 std::atomic<unsigned> users_{0};std::atomic<bool> managing_{false},external_busy_{false},invalidated_{false},refresh_space_{false};
 QueueHandle_t commands_=nullptr;TaskHandle_t task_=nullptr;
 enum class Kind:uint8_t{Format,Eject,Retry};struct Command{Kind kind;uint32_t epoch;};
 uint32_t last_check_=0;unsigned failures_=0;bool deferred_close_=false;
 void publish(State s){const auto old=state_.exchange(s);if(old!=s){epoch_.fetch_add(1);Serial.printf("STORAGE state=%u epoch=%u card=%uMB\n",unsigned(s),epoch_.load(),capacity_mb_.load());}}
 void releaseNative(){
  _impl->mountpoint(nullptr);if(drive_!=FF_DRV_NOT_USED){if(fat_)f_mount(nullptr,volume_,0);ff_diskio_unregister(drive_);drive_=FF_DRV_NOT_USED;}
  if(vfs_){esp_vfs_fat_unregister_path("/sdcard");vfs_=false;}fat_=nullptr;mounted_=false;
  if(host_){sdmmc_host_deinit();host_=false;}free(card_);card_=nullptr;capacity_mb_=total_mb_=free_mb_=0;
 }
 bool initRaw(bool onebit){
  sdmmc_host_t h=SDMMC_HOST_DEFAULT();h.slot=SDMMC_HOST_SLOT_1;h.flags=onebit?SDMMC_HOST_FLAG_1BIT:SDMMC_HOST_FLAG_4BIT;h.max_freq_khz=onebit?4000:SDMMC_FREQ_DEFAULT;
  sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();slot.width=onebit?1:4;slot.clk=gpio_num_t(SD_CLK);slot.cmd=gpio_num_t(SD_CMD);slot.d0=gpio_num_t(SD_D0);slot.d1=gpio_num_t(SD_D1);slot.d2=gpio_num_t(SD_D2);slot.d3=gpio_num_t(SD_D3);slot.flags|=SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
  if(sdmmc_host_init()!=ESP_OK)return false;host_=true;
  card_=static_cast<sdmmc_card_t*>(calloc(1,sizeof(sdmmc_card_t)));
  if(!card_||sdmmc_host_init_slot(h.slot,&slot)!=ESP_OK||sdmmc_card_init(&h,card_)!=ESP_OK){releaseNative();return false;}
  auto* sector=static_cast<uint8_t*>(heap_caps_malloc(512,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA));
  const bool read=sector&&card_->csd.sector_size==512&&sdmmc_read_sectors(card_,sector,0,1)==ESP_OK;heap_caps_free(sector);
  if(!read){releaseNative();return false;}capacity_mb_=uint64_t(card_->csd.capacity)*card_->csd.sector_size/(1024*1024);return true;
 }
 void cacheSize(){
  DWORD free_clusters=0;FATFS* fs=nullptr;if(f_getfree(volume_,&free_clusters,&fs)==FR_OK&&fs){const uint64_t cluster=uint64_t(fs->csize)*512;total_mb_=uint64_t(fs->n_fatent-2)*cluster/(1024*1024);free_mb_=uint64_t(free_clusters)*cluster/(1024*1024);}
 }
 bool attachFs(){
  if(ff_diskio_get_drive(&drive_)!=ESP_OK){publish(State::Unreadable);return false;}
  volume_[0]='0'+drive_;ff_diskio_register_sdmmc(drive_,card_);ff_sdmmc_set_disk_status_check(drive_,true);
  esp_vfs_fat_conf_t conf{};conf.base_path="/sdcard";conf.fat_drive=volume_;conf.max_files=8;
  if(esp_vfs_fat_register_cfg(&conf,&fat_)!=ESP_OK){ff_diskio_unregister(drive_);drive_=FF_DRV_NOT_USED;publish(State::Unreadable);return false;}vfs_=true;
  const FRESULT result=f_mount(fat_,volume_,1);
  if(result==FR_OK&&(fat_->fs_type==FS_FAT32||fat_->fs_type==FS_FAT16||fat_->fs_type==FS_FAT12)){mounted_=true;_impl->mountpoint("/sdcard");cacheSize();publish(State::Ready);return true;}
  f_mount(nullptr,volume_,0);mounted_=false;_impl->mountpoint(nullptr);
  publish(result==FR_NO_FILESYSTEM||result==FR_INT_ERR||result==FR_OK?State::NeedsFormat:State::Unreadable);return false;
 }
 void probe(){releaseNative();publish(State::Checking);if(!initRaw(false)&&!initRaw(true)){publish(State::Absent);return;}if(!attachFs()&&state_==State::Unreadable){releaseNative();if(initRaw(true))attachFs();else publish(State::Absent);}}
 bool beginManagement(){if(managing_.exchange(true))return false;if(users_.load()||external_busy_.load()){managing_=false;return false;}return true;}
 void formatConfirmed(uint32_t token){
  const auto before=state_.load();if(!camera::may_format(before,false,true,token,epoch_.load())||!card_){Serial.println("STORAGE format rejected: stale confirmation/card unavailable");format_result_=FormatResult::CardChanged;return;}
  const auto cid=card_->cid;const uint32_t sectors=card_->csd.capacity;
  // Re-initialize and compare CID immediately before the first destructive write.
  // Swapping cards after opening the confirmation cannot format the new card.
  releaseNative();publish(State::Checking);
  if(!initRaw(false)&&!initRaw(true)){publish(State::Absent);format_result_=FormatResult::Failed;return;}
  if(memcmp(&cid,&card_->cid,sizeof(cid))||sectors!=card_->csd.capacity){attachFs();Serial.println("STORAGE format cancelled: card changed");format_result_=FormatResult::CardChanged;return;}
  if(!attachFs()&&state_!=State::NeedsFormat){format_result_=FormatResult::Failed;return;}
  publish(State::Formatting);_impl->mountpoint(nullptr);f_mount(nullptr,volume_,0);mounted_=false;
  void* work=heap_caps_malloc(16384,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);bool ok=false;
  if(work){const LBA_t partitions[]={100,0,0,0};FRESULT r=f_fdisk(drive_,partitions,work);
   MKFS_PARM options{};options.fmt=capacity_mb_.load()>=64?FM_FAT32:FM_FAT;options.n_fat=2;
   uint32_t unit=512;const uint64_t bytes=uint64_t(card_->csd.capacity)*512;while(unit<32768&&uint64_t(unit)*2*131072<=bytes)unit*=2;options.au_size=unit;
   if(r==FR_OK)r=f_mkfs(volume_,&options,work,16384);heap_caps_free(work);
   ok=r==FR_OK&&f_mount(fat_,volume_,1)==FR_OK;
  }
  if(ok){mounted_=true;_impl->mountpoint("/sdcard");cacheSize();publish(State::Ready);Serial.println("STORAGE format complete");format_result_=FormatResult::Done;}
  else{publish(State::Unreadable);Serial.println("STORAGE format failed; card remains unmounted");format_result_=FormatResult::Failed;}
 }
 static void workerEntry(void* p){static_cast<CameraStorage*>(p)->worker();}
 void worker(){
  probe();last_check_=millis();
  for(;;){Command cmd{};const bool received=xQueueReceive(commands_,&cmd,pdMS_TO_TICKS(200))==pdTRUE;
   if(!beginManagement()){if(received)xQueueSendToFront(commands_,&cmd,0);continue;}
   if(received){
    if(cmd.kind==Kind::Format)formatConfirmed(cmd.epoch);
    else if(cmd.kind==Kind::Eject){if(card_){_impl->mountpoint(nullptr);if(fat_)f_mount(nullptr,volume_,0);mounted_=false;publish(State::Ejected);}else publish(State::Absent);}
    else if(state_!=State::Ejected)probe();
   }else if(invalidated_.exchange(false)){publish(State::Unreadable);deferred_close_=true;last_check_=millis();}
   else if(state_==State::Ready&&refresh_space_.exchange(false)){cacheSize();}
   else if(uint32_t(millis()-last_check_)>=2000){last_check_=millis();
    if(deferred_close_){releaseNative();deferred_close_=false;publish(State::Absent);}
    else if(card_){if(sdmmc_get_status(card_)!=ESP_OK){if(++failures_>=2){releaseNative();publish(State::Absent);failures_=0;}}else failures_=0;}
    else probe();
   }
   managing_=false;
  }
 }
 public:
 CameraStorage():FS(fs::FSImplPtr(new VFSImpl())){}
 bool start(){if(task_)return true;commands_=xQueueCreate(4,sizeof(Command));if(!commands_)return false;publish(State::Checking);return xTaskCreatePinnedToCore(workerEntry,"Card manager",8192,this,1,&task_,1)==pdPASS;}
 bool ready()const{return state_==State::Ready&&!managing_;}State state()const{return state_.load();}uint32_t epoch()const{return epoch_.load();}
 uint32_t capacityMB()const{return capacity_mb_.load();}uint32_t totalMB()const{return total_mb_.load();}uint32_t freeMB()const{return free_mb_.load();}
 uint64_t totalBytes()const{return uint64_t(totalMB())*1024*1024;}uint64_t usedBytes()const{return uint64_t(totalMB()-freeMB())*1024*1024;}
 void busy(bool busy){external_busy_=busy;}void invalidate(){invalidated_=true;}
 bool acquire(uint32_t wait_ms=5){const uint32_t start=millis();for(;;){users_.fetch_add(1);if(ready())return true;users_.fetch_sub(1);if(state_!=State::Ready||uint32_t(millis()-start)>=wait_ms)return false;delay(1);}}void release(){users_.fetch_sub(1);}
 bool confirmFormat(uint32_t token){if(!commands_||!camera::may_format(state_,external_busy_.load()||users_.load(),true,token,epoch_))return false;Command c{Kind::Format,token};format_result_=FormatResult::Pending;if(xQueueSend(commands_,&c,0)==pdTRUE)return true;format_result_=FormatResult::None;return false;}
 FormatResult formatResult()const{return format_result_.load();}
 bool eject(){Command c{Kind::Eject,epoch_};return commands_&&xQueueSend(commands_,&c,0)==pdTRUE;}
 void refreshSpace(){refresh_space_=true;}
 void retry(){Command c{Kind::Retry,epoch_};if(commands_)xQueueSend(commands_,&c,0);}
};
inline CameraStorage camera_storage;
class StorageUse {
 bool held_;
 public:StorageUse():held_(camera_storage.acquire()){}~StorageUse(){if(held_)camera_storage.release();}explicit operator bool()const{return held_;}
};
