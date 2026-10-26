#include "depth_pass.h"

typedef struct
{
	mat4_f32 projection;
	VkDeviceAddress instance_transform_buffer_ptr;
	VkDeviceAddress visibility_id_buffer;
} push_constants;

void depth_pass_record(
	VkCommandBuffer cmd, 
	depth_pass *pass, 
	gpu_context *gpu, 
	arena *pf_arena, 
	render_geometry *geometry,
	gpu_texture *out_depth[static 1][frames_in_flight],
	u32 command_count,
	render_command (*commands)[frames_in_flight],
	camera *cam)
{
	if(!pass->render_state.pipeline)
	{
		pass->render_state = render_state_make(	
			.gpu = gpu,
			.temp_arena = pf_arena,	
			.fragment_shader_path = str8_lit("assets/shaders/depth_pass.frag.spv"),
			.vertex_shader_path	= str8_lit("assets/shaders/depth_pass.vert.spv"),
			.depth_write = true,
			.depth_test = true,
            .depth_compare_op = VK_COMPARE_OP_LESS,
			.render_state_dynamic_state(
				VK_DYNAMIC_STATE_VIEWPORT, 
				VK_DYNAMIC_STATE_SCISSOR
			),
			.depth_target_format = out_depth[0][gpu->frame_index]->format,
			.render_state_push_constants(push_constants, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT),
			.render_state_vertex_bindings(
				render_vertex_binding[render_vertex_attribute_position]
			)
		);
	}

	vku_cmd_set_viewport_and_scissors(cmd, gpu->swapchain.extent);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->render_state.pipeline);			

	vku_cmd_bind_buffers(
		.cmd = cmd,
		.index_buffer = geometry->indices->handle,
		.vku_vertex_buffers(
			[render_vertex_attribute_position] = { .buffer = geometry->positions->handle }			
		)
	);

	vku_cmd_begin_rendering(
		.cmd = cmd,
		.temp_mem = pf_arena,
		.depth_attachment = out_depth[0][gpu->frame_index],
		.extent = gpu->swapchain.extent,
		.clear_color = {0,0,0,1},
		.depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.depth_store_op = VK_ATTACHMENT_STORE_OP_STORE
	);

	for each_index(cmd_id, command_count)
	{
		render_command *command = commands[cmd_id] + gpu->frame_index;
		render_batch *batch = command->batch;
	
		push_constants pc = {		
			.projection = cam->proj_view,
			.instance_transform_buffer_ptr = batch->transform_buffer->device_address,
			.visibility_id_buffer = command->visible_id_buffer->device_address
		};
				
		render_state_cmd_push_constants(
			cmd, 
			&pass->render_state, 
			&pc
		);
		
		vku_cmd_mdi(cmd, command->indirect_commands_buffer, command->indirect_command_count);	
	}

	vkCmdEndRendering(cmd);	
}
void depth_pass_release(depth_pass *pass, gpu_context *gpu)
{
	if(pass->render_state.pipeline)
	{
		render_state_release(&pass->render_state, gpu);
		pass->render_state.pipeline = 0;
	}
}
