#include "ui.h"
#include "vk_shader.h"
#include <jahoda/base/font.h>
#include <jahoda/base/tick_info.h>

typedef struct
{
	mat4_f32 projection;
    vec3_f32 text_color;
    f32 scale;
	vec2_f32 pos;
	vec2_f32 padding; 
} push_constants;

ui ui_make(arena pf_arena, gpu_context *gpu, font_manager *fonts, font_id font_id)
{	
	strv vertex_shader_path = strv_from_cstr("assets/shaders/text.vert.spv");
	strv fragment_shader_path = strv_from_cstr("assets/shaders/text.frag.spv");
	
	ui out = {0};

	out.pf_arena = pf_arena;

	out.fonts = fonts;
	out.font_id = font_id;

	VkImageCreateInfo dummy_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM, 
		.extent = {1, 1, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage dummy_image;
	vk_check(vkCreateImage(gpu->device.handle, &dummy_ci, NULL, &dummy_image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu->device.handle, dummy_image, &mem_reqs);

	u32 render_target_memory_index = vk_physical_device_pick_memory_type(
		&gpu->pdevice, 
		mem_reqs.memoryTypeBits,             
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  
	);

	vkDestroyImage(gpu->device.handle, dummy_image, NULL);

	// render target
	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
    {
        VkImageCreateInfo image_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM, // r8g8b8a8 for albedo
            .extent = { gpu->swapchain.extent.width, gpu->swapchain.extent.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        vk_check(vkCreateImage(gpu->device.handle, &image_ci, NULL, &out.albedo_images[i]));

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(gpu->device.handle, out.albedo_images[i], &mem_reqs);

        VkDeviceSize alloc = gpu_arena_push(
            gpu->static_dl_sw_img_arenas + i, 
            mem_reqs.size, 
            mem_reqs.alignment
        );

        vk_check(vkBindImageMemory(gpu->device.handle, out.albedo_images[i],  gpu->static_dl_sw_img_arenas[i].mem, alloc));

        VkImageViewCreateInfo view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = out.albedo_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_ci.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk_check(vkCreateImageView(gpu->device.handle, &view_ci, NULL, &out.albedo_views[i]));
    }

	// render_pass
	VkAttachmentDescription color_attachment = {
		.format = VK_FORMAT_R8G8B8A8_UNORM, // r8g8b8a8 for albedo
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref
	};

	VkSubpassDependency dependency = {
		.srcSubpass = 0,
		.dstSubpass = VK_SUBPASS_EXTERNAL,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};
	
	VkRenderPassCreateInfo present_render_pass_ci = { 
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, 
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
		.flags = 0,
	};
	vk_check(vkCreateRenderPass(gpu->device.handle, &present_render_pass_ci, NULL, &out.render_pass));
	// ---------------


	// framebuffers
	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
	{
		VkFramebufferCreateInfo framebuffer_ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = out.render_pass, 
			.attachmentCount = 1,            
			.pAttachments = &out.albedo_views[i], 
			.width = gpu->swapchain.extent.width,
			.height = gpu->swapchain.extent.height,
			.layers = 1,
		};
	
		vk_check(vkCreateFramebuffer(gpu->device.handle, &framebuffer_ci, NULL, &out.framebuffers[i]));
	}
	// ---------------

	// descriptor pool
	VkDescriptorPoolSize descriptor_pool_size = {
		.descriptorCount = 3,
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	
	};
	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = arrsize(out.descriptor_sets),
		.poolSizeCount = 1,
		.pPoolSizes = &descriptor_pool_size,
	};

	vk_check(vkCreateDescriptorPool(gpu->device.handle, &pool_info, NULL, &out.descriptor_pool));
	// ---------------

	// descriptor set layout
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
		.binding = 0,
		.descriptorType =VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = 1,
		.pBindingFlags = &binding_flags,
	};

	VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &binding_flags_info,
		.bindingCount = 1,
		.pBindings = &descriptor_set_layout_binding,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
	};

	vk_check(vkCreateDescriptorSetLayout(gpu->device.handle, &info, NULL, &out.descriptor_set_layout));
	// ---------------

	// descriptor sets
	VkDescriptorSetLayout layouts[jahoda_vk_frames_in_flight];
    for (u32 i = 0; i < jahoda_vk_frames_in_flight; i++) 
	{
        layouts[i] = out.descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = out.descriptor_pool,
        .descriptorSetCount = jahoda_vk_frames_in_flight,
        .pSetLayouts = layouts
    };

    vk_check(vkAllocateDescriptorSets(gpu->device.handle, &alloc_info, out.descriptor_sets));
	// ----------------

		VkSamplerCreateInfo sampler_ci = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST, 
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.maxAnisotropy = 1.0f,
			.minLod = 0.0f,
			.maxLod = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
		};
		vk_check(vkCreateSampler(gpu->device.handle, &sampler_ci, NULL, &out.font_sampler));


	for (uz i = 0; i < jahoda_vk_frames_in_flight; i++) 
    {
        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = out.fonts->entries[out.font_id].tex.view,
            .sampler = out.font_sampler
        };

        VkWriteDescriptorSet descriptor_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = out.descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &image_info
        };

        vkUpdateDescriptorSets(gpu->device.handle, 1, &descriptor_write, 0, NULL);
    }

	VkPushConstantRange push_constant_range = {
		.offset = 0,
		.size = sizeof(push_constants),
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDeviceSize max_text_buffer_size = sizeof(text_vertex)  *6  *1024; // enough for 1024 characters

	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
	{
		VkBufferCreateInfo vbo_ci = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = max_text_buffer_size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		vk_check(vkCreateBuffer(gpu->device.handle, &vbo_ci, NULL, &out.text_vbo[i]));

		VkMemoryRequirements mem_reqs;
		vkGetBufferMemoryRequirements(gpu->device.handle, out.text_vbo[i], &mem_reqs);

		VkDeviceSize alloc = gpu_arena_push(
			gpu->temp_hc_buf_arenas + i, 
			mem_reqs.size, 
			mem_reqs.alignment
		);		

		vk_check(vkBindBufferMemory(gpu->device.handle, out.text_vbo[i], gpu->temp_hc_buf_arenas[i].mem, alloc));
		out.text_vbo_ptrs[i] = gpu->temp_hc_buf_arenas[i].mapped_ptr + alloc;
	}	

	// pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = {
		.sType =  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &out.descriptor_set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant_range,
	};

	vk_check(vkCreatePipelineLayout(gpu->device.handle, &pipeline_layout_ci, NULL, &out.pipeline_layout));
	// ---------------

	// pipeline
	VkDynamicState dyn_state[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dyn_state_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dyn_state
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL
	};

	shader_bytes vertex_bytes = shader_bytes_from_file(
		out.pf_arena, 
		vertex_shader_path
	);

	shader_bytes fragment_bytes = shader_bytes_from_file(
		out.pf_arena,
		fragment_shader_path
	);

	out.vertex_shader = vk_shader_module_from_shader_bytes(&gpu->device, &vertex_bytes);
	out.fragment_shader = vk_shader_module_from_shader_bytes(&gpu->device, &fragment_bytes);

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_ci[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = out.vertex_shader,
			.pName = "main"
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = out.fragment_shader,
			.pName = "main"
		}
	};
		

	VkVertexInputBindingDescription binding_desc = {
		.binding = 0,
		.stride = sizeof(text_vertex), 
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription attr_descs[2] = {
		{ .binding = 0, .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(text_vertex, pos) },
		{ .binding = 0, .location = 1, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(text_vertex, uv) }
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pVertexBindingDescriptions = &binding_desc,
		.vertexBindingDescriptionCount = 1,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = attr_descs
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};   

	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable= VK_FALSE,
		.front = {0},
		.back = {0},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 0.0f,
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_TRUE, 
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
	};
	
	VkGraphicsPipelineCreateInfo pipeline_ci = { 
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = arrsize(pipeline_shader_stage_ci),
		.pStages = pipeline_shader_stage_ci,
		.pVertexInputState = &vertex_input_state,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dyn_state_ci,
		.layout = out.pipeline_layout,
		.renderPass = out.render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = NULL,
	};

	vk_check(vkCreateGraphicsPipelines(gpu->device.handle, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &out.pipeline));
	// ---------------	

	return out;
}

void ui_release(ui *ui, gpu_context *gpu)
{
	vkDestroySampler(gpu->device.handle, ui->font_sampler, NULL);
	vkDestroyPipeline(gpu->device.handle, ui->pipeline, NULL);
	vkDestroyPipelineLayout(gpu->device.handle, ui->pipeline_layout, NULL);
	vkDestroyDescriptorPool(gpu->device.handle, ui->descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(gpu->device.handle, ui->descriptor_set_layout, NULL);

	vkDestroyRenderPass(gpu->device.handle, ui->render_pass,  NULL);

	vkDestroyShaderModule(gpu->device.handle, ui->vertex_shader, NULL);
	vkDestroyShaderModule(gpu->device.handle, ui->fragment_shader, NULL);

	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
    {
		vkDestroyBuffer(gpu->device.handle, ui->text_vbo[i], NULL);
        vkDestroyImageView(gpu->device.handle, ui->albedo_views[i], NULL);
        vkDestroyImage(gpu->device.handle, ui->albedo_images[i], NULL);
    }

	// framebuffers
	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
	{
		vkDestroyFramebuffer(gpu->device.handle, ui->framebuffers[i], NULL);
	}	
}

void ui_record_draw(ui *ui, gpu_context *gpu, VkCommandBuffer cmd)
{
	if(gpu->swapchain_was_refreshed)
	{
		arena pf_arena = ui->pf_arena;
		font_manager *fonts = ui->fonts;
		font_id font_id = ui->font_id;

		ui_release(ui, gpu);

		*ui = ui_make(
			pf_arena, 
			gpu, 
			fonts, 
			font_id
		);
	}

	VkClearValue clear_value = {
		.color = { 0, 0, 0, 1 }
	};

	VkRenderPassBeginInfo rp_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.framebuffer = ui->framebuffers[gpu->frame_index],
		.clearValueCount = 1,
		.pClearValues = &clear_value,
		.renderPass  = ui->render_pass,
		.renderArea = { 
			.offset = {0,0}, 
			.extent =  gpu->swapchain.extent
		} 
	};

	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = gpu->swapchain.extent.width,
        .height = gpu->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = gpu->swapchain.extent,
    };

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipeline);

	vkCmdBindDescriptorSets(
		cmd, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		ui->pipeline_layout, 
		0,                                  
		1,                                  
		&ui->descriptor_sets[gpu->frame_index],
		0,                                  
		NULL                                
	);

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &ui->text_vbo[gpu->frame_index], offsets);

	u32 curr_offset = 0;

    da_foreach(&ui->nodes)
	{
		ui_node *it = ui->nodes.it;

		marker mark = arena_mark(ui->pf_arena);
		
		if(ui->nodes.it->type == ui_node_type_text)
		{			
			push_constants ps = {
				.text_color = ui->nodes.it->text.color,
				.projection = mat4_f32_ortho(
					.left = 0, 
					.right= gpu->swapchain.extent.width, 
					.bottom = gpu->swapchain.extent.height, 
					.top = 0, 
					.near_z = 0,
					.far_z = 1000
				),
				.scale = 1.f,
				.pos = {
					.x = 0,
					.y = 0
				}
			};

			vkCmdPushConstants(
				cmd, 
				ui->pipeline_layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(push_constants),		
				&ps
			);

			text_vertex_buffer verts = generate_text_vertices(
				ui->pf_arena, 
				&ui->fonts->entries[ui->font_id].altas, 
				strv_from_str(&ui->nodes.it->text.content), 
				ui->nodes.it->position.x, 
				ui->nodes.it->position.y
			);
		
			size_t copy_size = verts.occupied  *sizeof(text_vertex);
			memcpy(ui->text_vbo_ptrs[gpu->frame_index] + curr_offset  *sizeof(text_vertex), verts.data, copy_size);
			
			vkCmdDraw(cmd, verts.occupied, 1, curr_offset , 0);
			
			curr_offset += verts.occupied;
		}

		if(ui->nodes.it->type == ui_node_type_button)
		{		
			push_constants ps= {
				.text_color = it->button.color,
				.projection = mat4_f32_ortho(
					.left = 0, 
					.right= gpu->swapchain.extent.width, 
					.bottom = gpu->swapchain.extent.height, 
					.top = 0, 
					.near_z = 0,
					.far_z = 1000
				),
				.scale = 1.f,
				.pos = {
					.x = 0,
					.y = 0
				}
			};

			vkCmdPushConstants(
				cmd, 
				ui->pipeline_layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(push_constants),		
				&ps
			);
			
			text_vertex verts2[6] = {
				{ .pos = {it->position.x,  						it->position.y},   			.uv = {0,0} },
				{ .pos = {it->position.x,  						it->position.y - it->button.measured_size.y},   		.uv = {0,0} },
				{ .pos = {it->position.x + it->button.measured_size.x, 	it->position.y},  		.uv = {0,0} },
				{ .pos = {it->position.x + it->button.measured_size.x, 	it->position.y},  		.uv = {0,0} },
				{ .pos = {it->position.x,  						it->position.y - it->button.measured_size.y},   		.uv = {0,0} },
				{ .pos = {it->position.x + it->button.measured_size.x, 	it->position.y - it->button.measured_size.y},  .uv = {0,0} }
			};

			memcpy(
				ui->text_vbo_ptrs[gpu->frame_index] + curr_offset  *sizeof(text_vertex), 
				verts2, 
				sizeof(text_vertex)  *6
			);          

			vkCmdDraw(cmd, 6, 1, curr_offset , 0);

			curr_offset += 6;

			push_constants ps2 = {
				.text_color = ui->nodes.it->button.text.color,
				.projection = mat4_f32_ortho(
					.left = 0, 
					.right= gpu->swapchain.extent.width, 
					.bottom = gpu->swapchain.extent.height, 
					.top = 0, 
					.near_z = 0,
					.far_z = 1000
				),
				.scale = 1.f,
				.pos = {
					.x = 0,
					.y = 0
				}
			};

			vkCmdPushConstants(
				cmd, 
				ui->pipeline_layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(push_constants),		
				&ps2
			);

			text_vertex_buffer verts = generate_text_vertices(
				ui->pf_arena, 
				&ui->fonts->entries[ui->font_id].altas, 
				strv_from_str(&ui->nodes.it->text.content), 
				ui->nodes.it->position.x + 15, 
				ui->nodes.it->position.y - 30
			);
		
			size_t copy_size = verts.occupied  *sizeof(text_vertex);
			memcpy(ui->text_vbo_ptrs[gpu->frame_index] + curr_offset  *sizeof(text_vertex), verts.data, copy_size);
			
			vkCmdDraw(cmd, verts.occupied, 1, curr_offset , 0);
			
			curr_offset += verts.occupied;
		}

		arena_pop_to_marker(mark);
	}


    vkCmdEndRenderPass(cmd);	
}

void ui_refresh(ui *ui, ui_state new_state)
{
	if(!new_state.mouse_down && ui->state.mouse_down)
	{
		new_state.mouse_click = true;
	}

	ui->state = new_state;

	ui->nodes = (ui_node_da){0};

	// @todo: change magic 128 so something meaningful	
	da_reserve(ui->pf_arena, &ui->nodes, 256);
}

void ui_text(ui *ui, vec3_f32 color, vec2_f64 position, const u8 *fmt, ...)
{
	va_list list;
    va_start(list, fmt);

	// @explain: NULL makes sure that we dont resize but only use preallocated memory
	da_append(NULL, &ui->nodes, 
		(ui_node){
			.position = position,
			.type = ui_node_type_text,
			.parent = 0,
			.children = {0},
			.text = {
				.content = str_from_fmt_va(ui->pf_arena, fmt, list),
				.color = color
			}
		}
	);	

	va_end(list);
}


bool8 ui_button(ui *ui, vec3_f32 color, vec3_f32 text_color, vec2_f64 position, const u8 *fmt, ...)
{
	va_list list;
    va_start(list, fmt);

	// @explain: NULL makes sure that we dont resize but only use preallocated memory
	da_append(NULL, &ui->nodes, 
		(ui_node){
			.position = position,
			.type = ui_node_type_button,
			.parent = 0,
			.children = {0},
			.button = {
				.text = {
					.content = str_from_fmt_va(ui->pf_arena, fmt, list),
					.color = text_color
				},
				.color = color	
			}
		}
	);	

	va_end(list);

	ui_node *node = da_last(&ui->nodes);

	node->button.measured_size = (vec2_f64){
		.x = font_measure_text_width(&ui->fonts->entries[ui->font_id].altas, strv_from_str(&node->button.text.content)) + 30,
		.y = ui->fonts->entries[ui->font_id].altas.font_height + 15
	};

	bool is_hovered = (
		ui->state.mouse_pos.x >= node->position.x && 
		ui->state.mouse_pos.x <= (node->position.x + node->button.measured_size.x) &&
		ui->state.mouse_pos.y >= (node->position.y - node->button.measured_size.y) && 
		ui->state.mouse_pos.y <= (node->position.y)
	);

	if(is_hovered)
	{
		node->button.color = (vec3_f32){
			node->button.color.x + 0.2f, 
			node->button.color.y + 0.2f, 
			node->button.color.z + 0.2f
		}; 

		if(ui->state.mouse_down)
		{
			node->button.color = (vec3_f32){
				node->button.color.x + 0.3f, 
				node->button.color.y + 0.3f, 
				node->button.color.z + 0.3f
			}; 
		}
		else if(ui->state.mouse_click)
		{
			node->button.color = (vec3_f32){
				node->button.color.x + 0.5f, 
				node->button.color.y + 0.5f, 
				node->button.color.z + 0.5f
			}; 

			return true;
		}
	}

	return false;
}
