#pragma once
#include <stdlib.h>
#include <string.h>
#include <esp_heap_caps.h>
void doom_platform_exit(void) __attribute__((noreturn));
void doom_platform_fatal(const char* message) __attribute__((noreturn));
static inline void* doom_malloc(size_t n){void* p=heap_caps_malloc(n?n:1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!p)doom_platform_fatal("Game memory exhausted");return p;}
static inline void* doom_calloc(size_t n,size_t s){if(s&&n>SIZE_MAX/s)doom_platform_fatal("Invalid game allocation");void* p=doom_malloc(n*s);memset(p,0,n*s);return p;}
static inline void* doom_realloc(void* p,size_t n){if(!n){heap_caps_free(p);return NULL;}void* q=heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!q)doom_platform_fatal("Game memory exhausted");return q;}
#define malloc doom_malloc
#define calloc doom_calloc
#define realloc doom_realloc
#define free heap_caps_free
