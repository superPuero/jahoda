#ifndef jahoda_profile
#define jahoda_profile

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "platform_detect.h"
#include "types.h"
#include "da.h"
#include "utils.h"

typedef struct
{
	str name;
	f64 time;
} profile_entry;

da_declare(profile_entry, profiler);
#ifdef jahoda_platform_windows
#include <windows.h> 
#define profile_impl(arena, profiler, pname, ...)\
    LARGE_INTEGER pname##_freq, pname##_begin, pname##_end; \
    QueryPerformanceFrequency(&pname##_freq); \
    QueryPerformanceCounter(&pname##_begin); \
    da_append(arena, profiler, (profile_entry){.name = str_from_fmt(arena, __VA_ARGS__)}); \
    profile_entry *pname##entry = da_last(profiler); \
    for(u32 pname##i = 0; \
        pname##i < 1; \
        QueryPerformanceCounter(&pname##_end), \
        pname##entry->time = (f64)(pname##_end.QuadPart - pname##_begin.QuadPart) / (f64)pname##_freq.QuadPart, \
        ++pname##i)
#else
#define profile(arena, profiler, ...) while(0)
#endif




#define profile_trampoline(arena, profiler, pname, ...) profile_impl(arena, profiler, pname, __VA_ARGS__)
#define profile(arena, profiler, ...) profile_trampoline(arena, profiler, concat(_prf_, __COUNTER__), __VA_ARGS__)




#endif


