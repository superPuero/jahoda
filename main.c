#include <signal.h>

#include "src/base/env.h"
#include "src/base/ent.h"

#include "src/core/math.h"
#include "src/core/time.h"

#include "src/gfx/vk_instance.h"
#include "src/gfx/vk_device.h"
#include "src/gfx/vk_swapchain.h"
#include "src/core/math.h"

void janohda_main(int argc, char **argv);

int main(int argc, char** argv)
{
	janohda_main(argc, argv);
    return 0;
}

void poll_event(env* env)
{
	window_poll_events(&env->win);
	env->exit = window_should_close(&env->win);
}

void update(env* env)
{
	tick_info_update(&env->tick_info);

	ui_refresh(
		&env->ui, 
		(ui_state){
			.mouse_down = window_mouse_button_down(&env->win, GLFW_MOUSE_BUTTON_1), 
			.mouse_pos = window_mouse_pos(&env->win)
		}
	);

	ui_text(
		&env->ui, 
		strv_from_cstr("hello"), 
		!window_mouse_button_down(&env->win, GLFW_MOUSE_BUTTON_1) ? (vec3_f32){1,1,1} : (vec3_f32){1,1,0}, (vec2_f64){.x = 200, .y = 100}
	);	

	ui_text(
		&env->ui, 
		strv_from_cstr("jahoda"), 
		(vec3_f32){
			1,	
			1,
			1
		},
		env->ui.state.mouse_pos
	);

	str dt_text = str_from_fmt(&env->pf_arena, "dt: %.3f", env->tick_info.dt);
	str fps_text = str_from_fmt(&env->pf_arena, "fps: %.2f", 1.0f/env->tick_info.dt);
	str prev_frame_mem_usage = str_from_fmt(&env->pf_arena, "pf_arena(env) usage: %.2fKb", env->pf_arena_last_frame_usage);
	
	ui_text(
		&env->ui, 
		strv_from_str(&prev_frame_mem_usage), 
		(vec3_f32){1,1,1},
		(vec2_f64){.x = 200, .y = 300}
	);

	ui_text(
		&env->ui, 
		strv_from_str(&fps_text), 
		(vec3_f32){1,1,1}, (vec2_f64){.x = 200, .y = 400}
	);

	ui_text(
		&env->ui, 
		strv_from_str(&dt_text), 
		(vec3_f32){1,1,1},(vec2_f64){.x = 200, .y = 500}
	);	

}

void try_render(env* env)
{
	if(gpu_context_try_begin_frame(&env->gpu, &env->win))
	{
		VkCommandBuffer cmd = gpu_context_grab_graphics(&env->gpu);
		vkResetCommandBuffer(cmd, 0);
	
		VkCommandBufferBeginInfo begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
	
		vk_check(vkBeginCommandBuffer(cmd, &begin_info)); 		
		
		ui_record_draw(
			&env->ui, 
			&env->gpu, 
			cmd
		);
		
		present_pass_record(
			&env->gpu, 
			&env->present_pass, 
			env->ui.albedo_views, 
			cmd
		);
		
		vk_check(vkEndCommandBuffer(cmd));
	
		gpu_context_commit_graphics(&env->gpu, cmd);

		gpu_context_end_frame(&env->gpu);		
	}
}


// @thoughts: this is evil but this it is what it is
env* env_ptr = NULL;

void interrupt_handle(int sig)
{
	info("interrupt");

	if(env_ptr)
	{
		env_ptr->exit = true;
	}
}

void janohda_main(int argc, char **argv)
{
	env jahoda = env_make(argc, argv); 

	env_ptr = &jahoda;
	signal(SIGINT, interrupt_handle);

    while (!jahoda.exit)
    {
		poll_event(&jahoda);
		update(&jahoda);

		if(window_is_minimized(&jahoda.win)) { glfwWaitEvents(); }
		else { try_render(&jahoda); }

		jahoda.pf_arena_last_frame_usage = jahoda.pf_arena.current/1000.f;

		arena_reset(&jahoda.pf_arena);
    }

	env_release(&jahoda);
}

