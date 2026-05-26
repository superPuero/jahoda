#ifndef jahoda_profile
#define jahoda_profile

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "types.h"
#include "da.h"
#include "platform_detect.h"

typedef struct
{
	const char* name;
	f64 time;
} profile_entry;

da_declare(profile_entry, profiler);

#ifdef jahoda_platform_windows
#include <windows.h> 
#define profile(arena, profiler, pname) \
    LARGE_INTEGER pname##_freq, pname##_begin, pname##_end; \
    QueryPerformanceFrequency(&pname##_freq); \
    QueryPerformanceCounter(&pname##_begin); \
    da_append(arena, profiler, (profile_entry){.name = #pname}); \
    profile_entry* pname##entry = da_last(profiler); \
    for(u32 pname##i = 0; \
        pname##i < 1; \
        QueryPerformanceCounter(&pname##_end), \
        pname##entry->time = (f64)(pname##_end.QuadPart - pname##_begin.QuadPart) / (f64)pname##_freq.QuadPart, \
        ++pname##i)
#else
#define profile(arena, profiler, pname) while(0)
#endif

#endif


