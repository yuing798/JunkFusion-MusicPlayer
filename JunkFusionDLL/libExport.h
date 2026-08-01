#pragma once

// 1. 定义宏：判断当前是 Windows 还是类 Unix
#ifdef _WIN32
// Windows 平台
    #define LIB_EXPORT __declspec(dllexport)
#else
// Linux / macOS / MinGW 等 GCC/Clang 平台
    #define LIB_EXPORT __attribute__((visibility("default")))
#endif