#ifndef jahoda_basic_pass
#define jahoda_basic_pass

#define basic_pass_declare(pass_name, push_constants_type, shader_path)\
typedef struct\
{\
	render_state render_state; \
    VkDescriptorBufferBindingInfoEXT descriptor_binding_info[frames_in_flight];\
	gpu_buffer *descriptor_buffer[frames_in_flight];\
} pass_name;\
static inline void pass_name##_record(\
    VkCommandBuffer cmd,\
	pass_name *pass, \
	gpu_context *gpu, \
	arena *temp_mem, \
    gpu_buffer_arena *coherent_descriptor_buffer_arena,\
    f32 time_in_seconds,\
	gpu_texture *in_texture per_frame_in_flight, \
	gpu_texture *out_texture per_frame_in_flight,\
    push_constants_type pc)\
{\
	if(!pass->render_state.pipeline)\
	{\
		pass->render_state = render_state_make(	\
			.gpu = gpu,\
			.temp_arena = temp_mem,	\
			.fragment_shader_path = str8_lit(shader_path),\
			.vertex_shader_path	= str8_lit("assets/shaders/quad.vert.spv"),\
			.depth_write = false,\
			.depth_test = true,\
			.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL,\
			.render_state_dynamic_state(\
				VK_DYNAMIC_STATE_VIEWPORT, \
				VK_DYNAMIC_STATE_SCISSOR\
			),\
            .render_state_descriptor_bindings(\
				{\
					.binding = 0,\
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,\
					.descriptorCount = 1,\
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,\
					.pImmutableSamplers = NULL\
				}\
            ),\
			.render_state_color_targets(\
				[0] = out_texture[gpu->frame_index]->format,\
			),\
			.render_state_push_constants(\
				push_constants_type, \
				VK_SHADER_STAGE_FRAGMENT_BIT\
			)\
		);\
\
        for each_index(i, frames_in_flight)\
        {\
            pass->descriptor_buffer[i] = gpu_buffer_arena_push(\
                u8,\
                .arena = coherent_descriptor_buffer_arena,\
                .device = &gpu->device,\
                .tag = text128_make("deferred_light_pass.descriptor_buffer[%u]", i),\
                .size = pass->render_state.descriptor_layout_size\
            );\
        \
            pass->descriptor_binding_info[i] = vku_descriptor_buffer_binding_info_make(gpu, pass->descriptor_buffer[i]->handle);                    \
        };\
\
	}\
\
     vku_cmd_textures_transition_state(\
        cmd,\
        temp_mem, \
		{ in_texture[gpu->frame_index],  gpu_texture_state_fragment_read }\
    );\
\
    vku_coherent_descriptor_buffer_write_image(\
        gpu, \
        pass->descriptor_buffer[gpu->frame_index], \
        0, \
        0,\
        pass->render_state.descriptor_layout, \
        in_texture[gpu->frame_index]\
    );\
\
	vku_cmd_begin_rendering(\
		.cmd = cmd,\
		.temp_mem = temp_mem,\
		.vku_begin_render_color_attachments(\
			[0] = out_texture[gpu->frame_index], \
		),\
		.depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD,\
		.depth_store_op = VK_ATTACHMENT_STORE_OP_NONE,\
		.extent = gpu->swapchain.extent,\
		.clear_color = {0,0,0,1},\
	);\
\
    gpu->device.cmd_bind_descriptor_buffer(cmd, 1, &pass->descriptor_binding_info[gpu->frame_index]);        \
\
    gpu->device.cmd_set_descriptor_buffer_offset(\
        cmd,\
        VK_PIPELINE_BIND_POINT_GRAPHICS,\
        pass->render_state.pipeline_layout,\
        0, \
        1, \
        (u32[]){ 0 },\
        (VkDeviceSize[]){ 0 }\
    );\
\
	vku_cmd_set_viewport_and_scissors(cmd, gpu->swapchain.extent);	\
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->render_state.pipeline);\
	render_state_cmd_push_constants(\
		cmd, \
		&pass->render_state, \
		&pc\
	);\
\
	vkCmdDraw(cmd, 3, 1, 0, 0);\
\
	vkCmdEndRendering(cmd);	\
}\
\
void pass_name##_release(pass_name *pass, gpu_context *gpu)\
{\
	if(pass->render_state.pipeline)\
	{\
		render_state_release(&pass->render_state, gpu);\
		pass->render_state.pipeline = 0;\
	}\
}

#endif