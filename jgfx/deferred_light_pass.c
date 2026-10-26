#include "deferred_light_pass.h"

typedef struct
{
	mat4_f32 projection;
} deferred_light_push_constants;

void deferred_light_pass_record(
    VkCommandBuffer cmd,
    deferred_light_pass *pass, 
    gpu_context *gpu, 
    arena *temp_mem,
    gpu_buffer_arena *coherent_descriptor_buffer_arena,
    gpu_texture *in_depth per_frame_in_flight,
    gpu_texture *in_albedo per_frame_in_flight,
    gpu_texture *in_normal per_frame_in_flight,
    gpu_texture *out_light per_frame_in_flight,
    camera *cam)
{
    if(!pass->render_state.pipeline)
    {
        pass->render_state = render_state_make(	
			.gpu = gpu,
			.temp_arena = temp_mem,	
			.fragment_shader_path = str8_lit("assets/shaders/deferred_light.frag.spv"),
			.vertex_shader_path	= str8_lit("assets/shaders/deferred_light.vert.spv"),
			.depth_write = false,
			.depth_test = false,
			.cull_mode = VK_CULL_MODE_NONE,
			.render_state_dynamic_state(
				VK_DYNAMIC_STATE_VIEWPORT, 
				VK_DYNAMIC_STATE_SCISSOR
			),
			.render_state_color_targets(
				out_light[gpu->frame_index]->format 
			),
			.render_state_descriptor_bindings(
				{
					.binding = 0, // albedo
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = NULL
				},
				{
					.binding = 1, // normals
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = NULL
				},
				{
					.binding = 2, // depth
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = NULL
				}	
			),
			.render_state_push_constants(
                deferred_light_push_constants, 
                VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
            )
		);  

        for each_index(i, frames_in_flight)
        {
            pass->descriptor_buffer[i] = gpu_buffer_arena_push(
                u8,
                .arena = coherent_descriptor_buffer_arena,
                .device = &gpu->device,
                .tag = text128_make("deferred_light_pass.descriptor_buffer[%u]", i),
                .size = pass->render_state.descriptor_layout_size
            );
        
            pass->descriptor_binding_info[i] = vku_descriptor_buffer_binding_info_make(gpu, pass->descriptor_buffer[i]->handle);                    
        };
    }

    vku_cmd_textures_transition_state(
        cmd,
        temp_mem, 
		{ in_albedo[gpu->frame_index],  gpu_texture_state_fragment_read }, 
        { in_normal[gpu->frame_index], gpu_texture_state_fragment_read },
        { in_depth[gpu->frame_index], gpu_texture_state_depth_read_only },
	);

    vku_coherent_descriptor_buffer_write_image(
        gpu, 
        pass->descriptor_buffer[gpu->frame_index], 
        0, 
        0,
        pass->render_state.descriptor_layout, 
        in_albedo[gpu->frame_index]
    );

    vku_coherent_descriptor_buffer_write_image(
        gpu, 
        pass->descriptor_buffer[gpu->frame_index], 
        1, 
        0,
        pass->render_state.descriptor_layout, 
        in_normal[gpu->frame_index]
    );

    vku_coherent_descriptor_buffer_write_image(
        gpu, 
        pass->descriptor_buffer[gpu->frame_index], 
        2, 
        0,
        pass->render_state.descriptor_layout, 
        in_depth[gpu->frame_index]
    );

    vku_cmd_begin_rendering(
        .cmd = cmd,
        .temp_mem = temp_mem,
        .vku_begin_render_color_attachments(
            out_light[gpu->frame_index]
        ),
        .extent = vku_extent3d_as_2d(out_light[gpu->frame_index]->extent),
        .clear_color = {0,0,0,1},
    );
    
    vku_cmd_set_viewport_and_scissors(cmd, gpu->swapchain.extent);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->render_state.pipeline);

    deferred_light_push_constants pc = {		
        .projection = cam->proj_view
    };
    
    render_state_cmd_push_constants(
        cmd,
        &pass->render_state,
        &pc
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

void deferred_light_pass_release(deferred_light_pass *pass, gpu_context *gpu)
{
    if(pass->render_state.pipeline)
    {
        render_state_release(&pass->render_state, gpu);   
        pass->render_state.pipeline = 0;     
    }
}
