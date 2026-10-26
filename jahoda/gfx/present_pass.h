#ifndef jahoda_present_pass
#define jahoda_present_pass

#include "gpu_context.h"
#include "gpu_utils.h"
#include "render_state.h"


typedef struct
{
	render_state render_state;
	gpu_buffer *descriptor_buffer[frames_in_flight];
	VkDescriptorBufferBindingInfoEXT descriptor_binding_info[frames_in_flight];
} present_pass;

void present_pass_refresh(present_pass *pass, gpu_context *gpu, gpu_texture *input_textures per_frame_in_flight, arena *pf_arena);
void present_pass_release(present_pass *pass, gpu_context *gpu);

void present_pass_record(
	VkCommandBuffer cmd, 
	present_pass *pass, 
	gpu_context *gpu, 
	arena *pf_arena,
	gpu_buffer_arena *coherent_descriptor_buffer_arena,
	gpu_texture *input_textures per_frame_in_flight);

#endif