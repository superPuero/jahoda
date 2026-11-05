#include "profiler.h"

#include "platform_detect.h"

#ifdef jahoda_platform_windows

#include <windows.h>
#include <x86intrin.h>

LARGE_INTEGER perf_freq = {0};

void profiler_reset(profiler *prf)
{
    prf->scopes_occupied = 0;
}

profile_scope *profiler_scope_begin(profiler *prf, text64 name)
{
    if(perf_freq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&perf_freq);
    }

    dbg_verify(prf->scopes_occupied < profiler_max_scopes);
    profile_scope *out = prf->scopes + prf->scopes_occupied++;

    LARGE_INTEGER begin;
    
    QueryPerformanceCounter(&begin);
    
    out->name = name;
    out->start_ticks = begin.QuadPart;
    
    u32 ui;
    out->process_cycles = __rdtscp(&ui);

    return out;
}

void profiler_scope_end(profile_scope *prf)
{
    u32 ui;
    u64 end_cycles;
    LARGE_INTEGER end;
    
    end_cycles = __rdtscp(&ui);
    QueryPerformanceCounter(&end);
    
    prf->process_cycles = end_cycles - prf->process_cycles;    

    f64 elapsed_ticks = (f64)(end.QuadPart - prf->start_ticks);
    prf->time_ms = (elapsed_ticks / (f64)perf_freq.QuadPart) * 1000;
    prf->start_ticks = 0;
}

#else
#include <time.h>
#include <x86intrin.h> 

profile_scope profile_begin(text64 name)
{
    profile_scope out = {0};

    u32 ui;
    u64 start_cycles;
    struct timespec begin;

    start_cycles = __rdtscp(&ui);
    clock_gettime(CLOCK_MONOTONIC, &begin);

    out.name = name;
    
    out.start_ticks = (u64)begin.tv_sec * 1000000000ULL + (u64)begin.tv_nsec;
    out.process_cycles = start_cycles;

    return out;
}

void profile_end(profile_scope *prf)
{
    u32 ui;
    u64 end_cycles;
    struct timespec end;
    
    // Read cycles and time
    end_cycles = __rdtscp(&ui);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    prf->process_cycles = end_cycles - prf->process_cycles;    

    u64 end_ticks = (u64)end.tv_sec * 1000000000ULL + (u64)end.tv_nsec;
    
    f64 elapsed_ns = (f64)(end_ticks - prf->start_ticks);
    prf->time_ms = elapsed_ns / 1000000.0;
    
    prf->start_ticks = 0;
}
#endif   


