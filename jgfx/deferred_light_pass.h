#ifndef jahoda_deferred_light_pass
#define jahoda_deferred_light_pass

#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/gpu_release_queue.h>
#include <jahoda/gfx/render_manager.h>
#include <jahoda/gfx/gpu_utils.h>
#include <jahoda/gfx/render_state.h>

typedef struct
{
    render_state render_state;
    VkDescriptorBufferBindingInfoEXT descriptor_binding_info[frames_in_flight];
	gpu_buffer *descriptor_buffer[frames_in_flight];
} deferred_light_pass;

void deferred_light_pass_record(
    VkCommandBuffer cmd,
    deferred_light_pass *pass, 
    gpu_context *gpu, 
    arena *temp_mem,
    gpu_buffer_arena *coherent_descriptor_buffer_arena,
    gpu_texture *in_depth per_frame_in_flight,
    gpu_texture *in_albedo per_frame_in_flight,
    gpu_texture *in_normal per_frame_in_flight,
    gpu_texture *out_light per_frame_in_flight,
    camera *cam);

void deferred_light_pass_release(
    deferred_light_pass *pass, 
    gpu_context *gpu);

#endif