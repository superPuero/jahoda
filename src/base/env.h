#ifndef jahode_env
#define jahode_env

#include <src/base/ent.h>
#include <src/base/tick_info.h>
#include <src/base/font.h>

#include <src/gfx/window.h>
#include <src/gfx/gpu_context.h>
#include <src/gfx/present_pass.h>
#include <src/gfx/ui.h>
#include <src/gfx/font_manager.h>
#include <src/base/command_line_args.h>

typedef struct
{
	command_line_args command_line_args;
	arena st_arena;
	arena pf_arena;
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
void env_release(env* env);

#endif