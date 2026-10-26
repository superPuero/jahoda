#ifndef jahoda_depth_pass
#define jahoda_depth_pass


#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/render_manager.h>
#include <jahoda/gfx/gpu_release_queue.h>
#include <jahoda/gfx/render_state.h>

typedef struct
{
	render_state render_state; 
} depth_pass;

void depth_pass_refresh(depth_pass *pass, gpu_context *gpu, arena *pf_arena);

void depth_pass_record(
	VkCommandBuffer cmd, 
	depth_pass *pass, 
	gpu_context *gpu, 
	arena *pf_arena, 
	render_geometry *geometry,
	gpu_texture *out_depth[static 1][frames_in_flight],
	u32 command_count,
	render_command (*commands)[frames_in_flight],
	camera *cam);

void depth_pass_release(depth_pass *pass, gpu_context *gpu);


#endif