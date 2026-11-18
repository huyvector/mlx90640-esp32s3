#pragma once
#include "settings_codec.hpp"
inline std::atomic<uint8_t> board_temperature_unit{0};
inline camera::SettingsBytes board_saved_settings{};
inline bool board_have_saved_settings=false;
inline std::atomic<bool> board_save_settings_pending{false},board_home_pending{false},board_restart_pending{false};
inline camera::Settings board_current_settings() {
 camera::Settings s;
 s.interpolation=mlx_cnf.interpolation;s.box=mlx_cnf.box_size;s.refresh=board_requested_refresh.load();
 s.unit=board_temperature_unit.load();s.fps=board_requested_fps.load();s.refresh=camera::sensor_refresh_for_fps(s.fps);
 s.color=mlx_cnf.color_scheme;s.marker=mlx_cnf.marker_mode;s.automatic=mlx_cnf.range_auto;s.capture=mlx_cap.capture_mode;
 s.minimum=mlx_cnf.range_min;s.maximum=mlx_cnf.range_max;s.pick_x=te_pick.x;s.pick_y=te_pick.y;return s;
}
inline bool board_settings_save() {
 const auto s=board_current_settings();if(!camera::valid_settings(s))return false;
 const auto bytes=camera::encode_settings(s);
 if(board_have_saved_settings&&bytes==board_saved_settings)return true;
 Preferences p;if(!p.begin("ThermalCam",false))return false;
 const bool ok=p.putBytes("cfg",bytes.data(),bytes.size())==bytes.size();p.end();
 if(ok){board_saved_settings=bytes;board_have_saved_settings=true;}
 Serial.printf("SETTINGS %s: refresh=%u, scale=%u, box=%u, color=%u, capture=%u, target_fps=%u, unit=%u\n",ok?"saved":"write failed",s.refresh,s.interpolation,s.box,s.color,s.capture,s.fps,s.unit);
 return ok;
}
inline bool board_settings_load() {
 Preferences p;if(!p.begin("ThermalCam",true)){Serial.println("SETTINGS: first boot, using defaults.");return false;}
 camera::SettingsBytes bytes{};const bool read=p.getBytesLength("cfg")==bytes.size()&&p.getBytes("cfg",bytes.data(),bytes.size())==bytes.size();p.end();
 camera::Settings s;if(!read||!camera::decode_settings(bytes,s)){Serial.println("SETTINGS: invalid/version mismatch, using defaults.");return false;}
 mlx_cnf.interpolation=s.interpolation;mlx_cnf.box_size=s.box;mlx_cnf.refresh_rate=camera::sensor_refresh_for_fps(s.fps);
 mlx_cnf.color_scheme=s.color;mlx_cnf.marker_mode=s.marker;mlx_cnf.range_auto=s.automatic;
 mlx_cnf.range_min=s.minimum;mlx_cnf.range_max=s.maximum;mlx_cap.capture_mode=s.capture;
 te_pick.x=s.pick_x;te_pick.y=s.pick_y;board_temperature_unit=s.unit;board_select_fps(s.fps);
 board_saved_settings=bytes;board_have_saved_settings=true;
 Serial.printf("SETTINGS restored: refresh=%u, scale=%u, box=%u, color=%u, capture=%u, target_fps=%u, unit=%u\n",unsigned(mlx_cnf.refresh_rate),s.interpolation,s.box,s.color,s.capture,s.fps,s.unit);
 return true;
}
