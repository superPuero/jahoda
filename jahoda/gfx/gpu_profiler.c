#include "gpu_profiler.h"

gpu_profiler gpu_profiler_make(gpu_context *gpu)
{
    gpu_profiler out = {0};
    out.period_ns = gpu->device.pdevice->properties.limits.timestampPeriod;
    out.timestamp_mask = gpu->device.queue_graphics_timestamp_valid_bits >= 64 ? ~0ull : (bit(gpu->device.queue_graphics_timestamp_valid_bits) - 1);

    for each_index(i, frames_in_flight)
    {
        vkCreateQueryPool(
            gpu->device.handle, 
            &(VkQueryPoolCreateInfo){
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryCount = gpu_profiler_max_scopes,
                .queryType = VK_QUERY_TYPE_TIMESTAMP
            }, 
            NULL, 
            &out.pool_begin[i]
        );

        vkCreateQueryPool(
            gpu->device.handle, 
            &(VkQueryPoolCreateInfo){
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryCount = gpu_profiler_max_scopes,
                .queryType = VK_QUERY_TYPE_TIMESTAMP
            }, 
            NULL, 
            &out.pool_end[i]
        );
    }

    return out;
}

void gpu_profiler_resolve(gpu_profiler *prf, gpu_context *gpu)
{
    if(prf->scopes_occupied[gpu->frame_index])
    {
        gpu_check(
            vkGetQueryPoolResults(
                gpu->device.handle, 
                prf->pool_begin[gpu->frame_index], 
                0, 
                prf->scopes_occupied[gpu->frame_index],
                sizeof(u64) * prf->scopes_occupied[gpu->frame_index], 
                &prf->scope_begin_query[gpu->frame_index], 
                sizeof(u64), 
                VK_QUERY_RESULT_64_BIT
            )
        );
    
        gpu_check(
            vkGetQueryPoolResults(
                gpu->device.handle, 
                prf->pool_end[gpu->frame_index], 
                0, 
                prf->scopes_occupied[gpu->frame_index],
                sizeof(u64) * prf->scopes_occupied[gpu->frame_index], 
                &prf->scope_end_query[gpu->frame_index], 
                sizeof(u64), 
                VK_QUERY_RESULT_64_BIT
            )
        );
    
        for each_index(scope_id, prf->scopes_occupied[gpu->frame_index])
        {            
            u64 ticks = (prf->scope_end_query[gpu->frame_index][scope_id] - prf->scope_begin_query[gpu->frame_index][scope_id]) & prf->timestamp_mask;
            prf->scopes[gpu->frame_index][scope_id].time_ms = (u64)((f64)ticks * prf->period_ns) / 1000000.0f;                        
        }
    
    }    
}

void gpu_profiler_reset(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd)
{
    vkCmdResetQueryPool(cmd, prf->pool_begin[gpu->frame_index], 0, gpu_profiler_max_scopes);
    vkCmdResetQueryPool(cmd, prf->pool_end[gpu->frame_index], 0, gpu_profiler_max_scopes);

    prf->scopes_occupied[gpu->frame_index] = 0;    
}

u32 gpu_profiler_scope_begin(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd, text64 name)
{
    prf->scopes[gpu->frame_index][prf->scopes_occupied[gpu->frame_index]].name = name;
    
    vkCmdWriteTimestamp2(
        cmd, 
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        prf->pool_begin[gpu->frame_index], 
        prf->scopes_occupied[gpu->frame_index]
    );

    return prf->scopes_occupied[gpu->frame_index]++;
}

void gpu_profiler_scope_end(gpu_profiler *prf, gpu_context *gpu, VkCommandBuffer cmd, u32 id)
{
    vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, prf->pool_end[gpu->frame_index], id);
}

void gpu_profiler_release(gpu_profiler *prf, gpu_context *gpu)
{
    for each_index(i, frames_in_flight)
    {
        vkDestroyQueryPool(gpu->device.handle, prf->pool_begin[i], NULL);
        vkDestroyQueryPool(gpu->device.handle, prf->pool_end[i], NULL);
    }
}