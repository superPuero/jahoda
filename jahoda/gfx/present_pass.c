#include "present_pass.h"
#include <jahoda/gfx/gpu_shader.h>


void present_pass_release(present_pass *pass, gpu_context *gpu)
{
	if(pass->render_state.pipeline)
	{
		render_state_release(&pass->render_state, gpu);
	}
}

void present_pass_record(
	VkCommandBuffer cmd, 
	present_pass *pass, 
	gpu_context *gpu, 
	arena *pf_arena,
	gpu_buffer_arena *coherent_descriptor_buffer_arena,
	gpu_texture *input_textures per_frame_in_flight)	
{		
	if(!pass->render_state.pipeline)
	{
		pass->render_state = render_state_make(	
			.gpu = gpu,
			.temp_arena = pf_arena,	
			.fragment_shader_path = str8_lit("assets/shaders/quad.frag.spv"),
			.vertex_shader_path	= str8_lit("assets/shaders/quad.vert.spv"),
			.depth_write = false,
			.depth_test = false,
			.cull_mode = VK_CULL_MODE_NONE,
			.render_state_dynamic_state(
				VK_DYNAMIC_STATE_VIEWPORT, 
				VK_DYNAMIC_STATE_SCISSOR
			),
			.render_state_color_targets(
				gpu->swapchain.format 
			),
			.render_state_descriptor_bindings(
				{
					.binding = 0, // swapchain texture
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = NULL
				}	
			),				
		);  
		

		for each_index(i, frames_in_flight)
		{
			pass->descriptor_buffer[i] = gpu_buffer_arena_push(
				u8,
				.arena = coherent_descriptor_buffer_arena,
				.device = &gpu->device,
				.tag = text128_make("present_pass.descriptor_buffer[%u]", i),
				.size = pass->render_state.descriptor_layout_size
			);

			pass->descriptor_binding_info[i] = vku_descriptor_buffer_binding_info_make(gpu, pass->descriptor_buffer[i]->handle);
		}
	}

	vku_cmd_textures_transition_state(
		cmd, 
		pf_arena,
		{ input_textures[gpu->frame_index], gpu_texture_state_fragment_read }
	);
	
	vku_coherent_descriptor_buffer_write_image(
		gpu, 
		pass->descriptor_buffer[gpu->frame_index], 
		0, 
		0,
		pass->render_state.descriptor_layout, 
		input_textures[gpu->frame_index]
	);

	vku_cmd_set_viewport_and_scissors(cmd, gpu->swapchain.extent);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->render_state.pipeline);

	vku_cmd_begin_rendering(
		.cmd = cmd,
		.temp_mem = pf_arena,
		.extent = gpu->swapchain.extent,
		.vku_begin_render_color_attachments(
			gpu->swapchain.textures + gpu->swapchain_image_index
		),
		.clear_color = {1,0,0,1}
	);

	gpu->device.cmd_bind_descriptor_buffer(cmd, 1, &pass->descriptor_binding_info[gpu->frame_index]);        

	gpu->device.cmd_set_descriptor_buffer_offset(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pass->render_state.pipeline_layout,
		0, 
		1, 
		(u32[]){ 0 },
		(VkDeviceSize[]){ 0 }
	);

	vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);	
}