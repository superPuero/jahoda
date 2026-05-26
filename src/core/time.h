#ifndef jahoda_time
#define jahoda_time

#include "types.h" 

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <time.h>
    #include <unistd.h>
#endif

#include <stdint.h>

static inline f64 jahoda_time_now() 
{
#if defined(_WIN32)
    static uint64_t frequency = 0;
    if (frequency == 0) {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        frequency = li.QuadPart;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (f64)counter.QuadPart / (f64)frequency;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
#endif
}


#endif