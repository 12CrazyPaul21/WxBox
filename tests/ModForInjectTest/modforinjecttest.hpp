#pragma once
#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_MODFORINJECTTEST
#define MODFORINJECTTEST_PUBLIC __declspec(dllexport)
#else
#define MODFORINJECTTEST_PUBLIC __declspec(dllimport)
#endif
#else
#ifdef BUILDING_MODFORINJECTTEST
#define MODFORINJECTTEST_PUBLIC __attribute__((visibility("default")))
#else
#define MODFORINJECTTEST_PUBLIC
#endif
#endif

extern "C" void MODFORINJECTTEST_PUBLIC SayHiInject(char* str);
