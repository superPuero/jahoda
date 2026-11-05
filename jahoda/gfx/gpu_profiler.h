#ifndef jahoda_gpu_profiler
#define jahoda_gpu_profiler

#define gpu_profiler_max_scopes 64

#include "gpu_context.h"


typedef struct
{
    text64 name;
    f64 time_ms;
} gpu_profile_scope;

typedef struct
{
    VkQueryPool pool_begin[frames_in_flight];
    VkQueryPool pool_end[frames_in_flight];
    
    u32 scopes_occupied[frames_in_flight];
    gpu_profile_scope scopes[frames_in_flight][gpu_profiler_max_scopes];
    
    u64 scope_begin_query[frames_in_flight][gpu_profiler_max_scopes];    
    u64 scope_end_query[frames_in_flight][gpu_profiler_max_scopes];    

    f64 period_ns;
    u64 timestamp_mask;
} gpu_profiler;

gpu_profiler gpu_profiler_make(gpu_context *gpu);
void gpu_profiler_resolve(gpu_profiler *prf, gpu_context *gpu);
void gpu_profiler_reset(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd);
u32 gpu_profiler_scope_begin(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd, text64 name);
void gpu_profiler_scope_end(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd, u32 id);
void gpu_profiler_release(gpu_profiler *prf, gpu_context *gpu);

#define gpu_profile_scope(prf, gpu, cmd, ...)\
    for(u32 _prf_scope = gpu_profiler_scope_begin(prf, gpu, cmd, text64_make(__VA_ARGS__)) + 1; \
        _prf_scope; \
        _prf_scope = (gpu_profiler_scope_end(prf, gpu, cmd, _prf_scope - 1), 0))

#define gpu_profile_scope_fmt "[%s] %*s %.3lf ms"
#define gpu_profile_scope_arg(scope) (scope)->name.data, (i32)(sizeof((scope)->name) - strlen((scope)->name.data) - 1), "", (f64)(scope)->time_ms

#endif