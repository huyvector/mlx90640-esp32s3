#pragma once
#include <atomic>
inline std::atomic<bool> board_doom_active{false},board_doom_pending{false};
extern "C" void doom_platform_exit(void) __attribute__((noreturn));
extern "C" void doom_platform_fatal(const char* message) __attribute__((noreturn));
bool doom_start();
