#pragma once
#include <stdint.h>
namespace camera {
enum class StorageState:uint8_t {Absent,Checking,Ready,NeedsFormat,Unreadable,Ejected,Formatting};
inline bool may_format(StorageState state,bool busy,bool confirmed,uint32_t confirmation_epoch,uint32_t card_epoch){return confirmed&&!busy&&confirmation_epoch==card_epoch&&(state==StorageState::Ready||state==StorageState::NeedsFormat);}
inline bool should_offer_format(StorageState state){return state==StorageState::NeedsFormat;}
}
