#ifndef jahoda_gbuffer_pass
#define jahoda_gbuffer_pass

#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/render_manager.h>
#include <jahoda/gfx/gpu_release_queue.h>
#include <jahoda/gfx/gpu_utils.h>
#include <jahoda/gfx/render_state.h>

typedef struct
{
	render_state render_state; 
} gbuffer_pass;


void gbuffer_pass_record(
	VkCommandBuffer cmd, 
	gbuffer_pass *pass, 
	gpu_context *gpu, 
	arena *pf_arena, 
	render_geometry *geometry,
	gpu_texture *in_depth[frames_in_flight], 
	gpu_texture *out_albedo[frames_in_flight], 
	gpu_texture *out_normal[frames_in_flight], 
	u32 command_count,
	render_command (*commands)[frames_in_flight],
	camera *cam);
	
void gbuffer_pass_release(gbuffer_pass *pass, gpu_context *gpu);

#endif