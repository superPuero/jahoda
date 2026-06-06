#ifndef cuda_test_env
#define cuda_test_env

#include <jahoda/gfx/window.h>
#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/present_pass.h>
#include <jahoda/gfx/ui.h>
#include <jahoda/gfx/font_manager.h>
#include <jahoda/base/command_line_args.h>

typedef struct
{
	command_line_args command_line_args;
	arena st_arena;
	arena pf_arena;
	f32 pf_arena_last_frame_usage;
	bool8 exit;
	glfw_context glfw_ctx;
	window win;
	entity_manager ent_manager;
	font_atlas_da font_atlases;
	tick_info tick_info;
	gpu_context gpu;
	font_manager font_manager;
	ui ui;
	present_pass present_pass;
} env;

env env_make(int argc, char **argv);
void env_release(env *env);

#endif