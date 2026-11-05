#ifndef jahoda_profile
#define jahoda_profile

#include "types.h"
#include "da.h"

#define profiler_max_scopes 64

typedef struct
{
	text64 name;
    u64 process_cycles; 
    i64 start_ticks;
	f64 time_ms;
} profile_scope;

typedef struct
{
    u32 scopes_occupied;
    profile_scope scopes[profiler_max_scopes];
} profiler;

#define profile_scope_fmt "[%s] %*s %.3lf ms, %llu cpuc"
#define profile_scope_arg(scope) (scope)->name.data, (i32)(sizeof((scope)->name) - strlen((scope)->name.data) - 1), "",(scope)->time_ms, (scope)->process_cycles

void profiler_reset(profiler *prf);
profile_scope *profiler_scope_begin(profiler *prf, text64 name);
void profiler_scope_end(profile_scope *scope);

#define profile_scope(prf, ...)\
    for(profile_scope *_prf_scope = profiler_scope_begin(prf, text64_make(__VA_ARGS__)); \
        _prf_scope->start_ticks; \
        profiler_scope_end(_prf_scope))

#endif