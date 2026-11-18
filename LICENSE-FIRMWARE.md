# Licenses in this firmware

The original thermal-camera source retains its MIT license in `LICENSE`. The combined firmware contains GPL-2.0-or-later DoomGeneric engine code; its complete corresponding port source is included in `src/doom/`, with `COPYING` and `UPSTREAM.json`. Preserve the original notices when redistributing source or binaries and provide the corresponding source for the combined build.

- Thermal camera: embedded-kiddie/MLX90640, MIT.
- MLX90640 driver: Adafruit, BSD; bundled Melexis algorithms retain Apache-2.0 notices in their files.
- DoomGeneric engine and ESP32 adaptation: GPL-2.0-or-later, `src/doom/COPYING`.
- DOOM Shareware 1.9 Episode 1 data: copyright id Software, unmodified original IWAD embedded read-only. The game data is separate from the GPL engine license. Source location and cryptographic hashes are recorded in `src/doom/UPSTREAM.json`.
- Google Material Symbols Rounded: Apache-2.0, `ui_assets/material/LICENSE` and per-asset URLs in `sources.json`.
- Roboto: SIL Open Font License 1.1, `ui_assets/Roboto-OFL.txt`.
- Arduino-ESP32, ESP-IDF and LovyanGFX retain their respective upstream licenses.

The original shareware data license is reproduced in `src/doom/LICENSE-DOOM1` (archived copy: https://github.com/redox-os/freedoom/blob/master/LICENSE-DOOM1). The bundled data is unmodified and distributed without charge; it is not covered by the GPL engine license.
