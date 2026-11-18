// DoomGeneric WAD adapter, GPL-2.0-or-later. Read-only original shareware IWAD in flash.
#include "esp_alloc.h"
#include "w_file.h"
extern const unsigned char doom_wad_data[];
extern const unsigned int doom_wad_size;
extern wad_file_class_t stdc_wad_file;
static wad_file_t rom_wad;
static wad_file_t* W_ROM_Open(char* path){if(strcmp(path,"/rom/doom1.wad"))return NULL;rom_wad.file_class=&stdc_wad_file;rom_wad.mapped=(byte*)doom_wad_data;rom_wad.length=doom_wad_size;return &rom_wad;}
static void W_ROM_Close(wad_file_t* wad){}
static size_t W_ROM_Read(wad_file_t* wad,unsigned offset,void* buffer,size_t size){if(offset>wad->length)return 0;if(size>wad->length-offset)size=wad->length-offset;memcpy(buffer,wad->mapped+offset,size);return size;}
wad_file_class_t stdc_wad_file={W_ROM_Open,W_ROM_Close,W_ROM_Read};
