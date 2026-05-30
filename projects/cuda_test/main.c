#include <signal.h>

#include "jahoda/base/env.h"

void janohda_main(int argc, char **argv);

int main(int argc, char **argv)
{
	janohda_main(argc, argv);
    return 0;
}

void poll_event(env *env)
{
	window_poll_events(&env->win);
	env->exit = window_should_close(&env->win);
}

vec3_f32 col = {1,1,0};

u32 counter = 0;
u32 upgrade_cost = 10;
u32 adder = 1;

void update(env *env)
{
	tick_info_update(&env->tick_info);

	ui_begin(
		&env->ui, 
		(ui_state){
			.mouse_down = window_mouse_button_down(&env->win, GLFW_MOUSE_BUTTON_1), 
			.mouse_pos = window_mouse_pos(&env->win)
		}
	);

	_sleep(1000.0f/150);

	ui_text(
		&env->ui, 
		(vec3_f32){ 1, 1, 1 }, 
		(vec2_f64){ .x = 200, .y = 100 },
		"Money: %lu", counter
	);

	if(ui_button(
		&env->ui, 
		(vec3_f32){ 0.1, 0.1, 0.1 },
		(vec3_f32){ 1, 1, 1 },
		(vec2_f64){ .x = 200, .y = 230 },
		"Click: %lu", adder 
	))
	{
		counter += adder;
	}

	if(ui_button(
		&env->ui, 
		(vec3_f32){ 0.1, 0.1, 0.1 },
		(vec3_f32){ 1, 1, 1 },
		(vec2_f64){ .x = 200, .y = 350 },
		"Upgrade X2 click: %lu", upgrade_cost 
	))
	{
		if(counter >= upgrade_cost)
		{
			counter -= upgrade_cost;
			adder *= 2;
			upgrade_cost *= 1.9f;
		}
	}



}

void try_render(env *env)
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
env *env_ptr = NULL;

void interrupt_handle(int sig)
{
	info("interrupt");

	if(env_ptr) // @todo: its ub because its no atomic_sig_t but idc for now
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

