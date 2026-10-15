#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <chrono>
#include <cstdio>
#include <cstdarg>

inline HANDLE PHANDLE = nullptr;

inline void dbgLog(const char* fmt, ...) {
    static auto t0 = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
    FILE* f = fopen("/tmp/cursor_debug.log", "a");
    if (!f) return;
    fprintf(f, "[%6lld ms] ", ms);
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fprintf(f, "\n");
    fclose(f);
}
