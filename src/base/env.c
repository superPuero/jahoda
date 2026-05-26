#include "env.h"

// @todo: change magic numbers
env env_make(int argc, char **argv)
{
	env out = {0};
	out.command_line_args = command_line_args_parse(argc, argv);

	if(out.command_line_args.h)
	{
		command_line_args_display();
		out.exit = true;
		return out;
	}

	out.exit = false;

	out.pf_arena = arena_make(
		.capacity = Mb(1),
		.name = strv_from_cstr("pf_arena(env)")
	);

	out.st_arena = arena_make(
		.capacity = Mb(1),
		.name = strv_from_cstr("st_arena(env)")
	);	
	
	
	out.ent_manager = entity_manager_make(
		.memory = &out.st_arena,
		.capacity = 1024
	);
	
	out.glfw_ctx = glfw_context_make();
	out.win = window_make(
		.width = out.command_line_args.ww,
		.height = out.command_line_args.wh,
		.title = strv_from_cstr("jahoda")
	);

	out.gpu = gpu_context_make(
		&out.pf_arena, 
		&out.st_arena, 
		&out.win
	);

	font_id source_code_pro_font_id = font_manager_load(
		&out.font_manager, 
		&out.gpu, 
		&out.st_arena, 
		&out.pf_arena, 
		strv_from_cstr("assets/fonts/SourceCodePro.ttf")
	);

	out.ui = ui_make(
		&out.pf_arena,
		&out.gpu,
		&out.font_manager,
		source_code_pro_font_id
	);
	
	out.present_pass = present_pass_make(
		&out.pf_arena,
		&out.gpu, 
		out.ui.albedo_views
	);	
	
	// @explain: window is invisible by default
	window_make_visible(&out.win);

	return out;
}

void env_release(env* env)
{
	if(env->command_line_args.h) { return; }
	vk_check(vkDeviceWaitIdle(env->gpu.device.handle));

	font_manager_release(&env->font_manager, &env->gpu);

	present_pass_release(
		&env->gpu, 
		&env->present_pass
	);

	ui_release(
		&env->ui,
		&env->gpu
	);

	gpu_context_release(&env->gpu);

	window_release(&env->win);
	glfw_context_release(&env->glfw_ctx);
	entity_manager_release(&env->ent_manager);
	arena_release(&env->st_arena);
	arena_release(&env->pf_arena);
	env->exit = true;
}