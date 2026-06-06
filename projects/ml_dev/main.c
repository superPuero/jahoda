#include <signal.h>

#include "env.h"
#include "galaxy_nn.h"
#include "cee.h"
#include <jahoda/core/time.h>
#include <jahoda/core/tensor.h>


void nn_main(int argc, char **argv);
void galaxy_main(int argc, char **argv);

int main(int argc, char **argv)
{			
	nn_main(argc, argv);
    return 0;
}

void poll_event(env *env)
{
	window_poll_events(&env->win);
	env->exit = window_should_close(&env->win);
}

f64 pred[10] = {0};
f64 loss[10] = {0};

vec3_f32 pred_color[10] = {0};

uz total_steps = 80000;
uz curr_step = 0;

bool8 learn = false;

void update(env *env)
{
	tick_info_update(&env->tick_info);

	ui_refresh(
		&env->ui, 
		(ui_state){
			.mouse_down = window_mouse_button_down(&env->win, GLFW_MOUSE_BUTTON_1), 
			.mouse_pos = window_mouse_pos(&env->win)
		}
	);

	if(learn)
	{
		if(curr_step < total_steps)
		{
			marker m = arena_mark(&env->pf_arena);
			uz random_class = rand() % 10;
			uz random_sample = rand() % 200;
			cee_nn_train_with(&env->nn, &env->pf_arena, &env->dataset[random_class][random_sample], random_class);
			arena_pop_to_marker(m);
			curr_step++;
		}
	}

	if(ui_button(
		&env->ui,
		(vec3_f32){0.2,0.2,0.2},
		learn ? (vec3_f32){0.9,0.6,0.6} : (vec3_f32){0.6,0.9,0.6},
		(vec2_f64){800, 300},
		learn ? "stop learning" : "start learning"
	))
	{
		learn = !learn;
	}

	if(ui_button(
		&env->ui,
		(vec3_f32){0.2,0.2,0.2},
		(vec3_f32){0.9,0.8,0.8},
		(vec2_f64){800, 500},
		"try predict"
	))
	{
		for(uz i = 0; i < 10; i++)
		{
			uz sample_index = rand() % 200;

			marker m = arena_mark(&env->pf_arena);
			
			tensor_f64 curr_pred = cee_nn_predict(&env->nn, &env->pf_arena, env->dataset[i] + sample_index);

			loss[i] = *mat_f64_at(&curr_pred, 0, i);

			f64 curr_pred_val = *tensor_at(&curr_pred, 0, i);
	
			pred_color[i] = (pred[i] > curr_pred_val) ? (vec3_f32){1,0.7,0.7} : (vec3_f32){0.7,1,0.7},
			
			pred[i] = curr_pred_val;
			
			arena_pop_to_marker(m);
		}	
	}

	ui_text(
		&env->ui,
		(vec3_f32){1,1,1},
		(vec2_f64){800, 100},
		"step %zu", curr_step
	);

	for(uz i = 0; i < 10; i++)
	{
		ui_text(
			&env->ui,
			pred_color[i],
			(vec2_f64){50, 100 + 100 * i},
			"%zu: %.2lf (%.2lf)", i, pred[i], loss[i] * 10
		);
	}


	
	if(ui_button(
		&env->ui,
		(vec3_f32){0.2,0.2,0.2},
		(vec3_f32){0.6,0.6,0.6},
		(vec2_f64){800, 700},
		"load from file"
	))
	{
		env->nn = cee_nn_load_from_file(&env->st_arena, &env->pf_arena, strv_from_cstr("model.bin"));
	}

	if(ui_button(
		&env->ui,
		(vec3_f32){0.2,0.2,0.2},	
		(vec3_f32){0.6,0.6,0.6},
		(vec2_f64){800, 900},
		"dump to file"
	))
	{
		cee_nn_dump_to_file(&env->nn, &env->pf_arena, strv_from_cstr("model.bin"));
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


env *env_ptr = NULL;

void interrupt_handle(int sig)
{
	info("interrupt");

	if(env_ptr) // @todo: its ub because its no atomic_sig_t but idc for now
	{
		env_ptr->exit = true;
	}
}

void nn_main(int argc, char **argv)
{
	env jahoda = env_make(argc, argv); 
	
	for(uz i = 0; i < 10; i++)
	{			
		for(uz j = 0; j < 200; j++)
		{
			marker m = arena_mark(&jahoda.pf_arena);
			str path = str_from_fmt(&jahoda.pf_arena, "projects/ml_dev/digit_dataset2/%zu/digit_%zu_%zu.png", i, i, j);
			jahoda.dataset[i][j] = load_image_to_tensor(&jahoda.st_arena, &jahoda.pf_arena, strv_from_str(&path));
			arena_pop_to_marker(m);
		}
	}

	env_ptr = &jahoda;
	signal(SIGINT, interrupt_handle);

	while (!jahoda.exit)
    {
		poll_event(&jahoda);
		update(&jahoda);

		if (!window_is_minimized(&jahoda.win)) { try_render(&jahoda); }

		// if(window_is_minimized(&jahoda.win)) { glfwWaitEvents(); }
		// else { try_render(&jahoda); }

		jahoda.pf_arena_last_frame_usage = jahoda.pf_arena.current/1000.f;

		arena_reset(&jahoda.pf_arena);
    }

	env_release(&jahoda);
}

void galaxy_main(int argc, char **argv)
{
	env jahoda = env_make(argc, argv); 
	
	galaxy_dataset galaxy = galaxy_dataset_from_csv(&jahoda.st_arena, &jahoda.pf_arena, strv_from_cstr("projects/ml_dev/galaxydataset.csv"));

	galaxy.celestials.occupied = 30000;
	galaxy_nn_model model = {.learning_rate = 0.001};

	uz bad = 0;

	for(uz i = 0; i < 100; i++)
	{
		galaxy_nn_train_round(&galaxy, &model);
	}
	
	galaxy.celestials.occupied = 53333;

	{
		uz i = 0;
		da_foreach(&galaxy.celestials)
		{
			if(softmax(&galaxy, &model, celestial_type_galaxy, i)> 0.5)
			{
				if(galaxy.celestials.it->class != celestial_type_galaxy)
				{
					bad++;
				}
			}
			if(softmax(&galaxy, &model, celestial_type_quasar, i) > 0.5)
			{
				if(galaxy.celestials.it->class != celestial_type_quasar)
				{
					bad++;
				}
			}
			if(softmax(&galaxy, &model, celestial_type_star, i) > 0.5)
			{
				if(galaxy.celestials.it->class != celestial_type_star)
				{
					bad++;
				}
			}
			i++;
		}
	}

	info("true rate: %.2lf", 100 - 100 / ((f64)galaxy.celestials.occupied / (f64)bad));

	uz pred_id = 16;

	info("prediction star = %.2lf", softmax(&galaxy, &model, celestial_type_star, 0));
	info("prediction galaxy = %.2lf", softmax(&galaxy, &model, celestial_type_galaxy, 0));
	info("prediction quasar = %.2lf", softmax(&galaxy, &model, celestial_type_quasar, 0));	
}

