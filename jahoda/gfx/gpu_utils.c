#include "gpu_utils.h"

typedef struct
{
    VkPipelineStageFlags2 stage;
    VkAccessFlags2 access;
    VkImageLayout layout;
} gpu_texture_state_translation;

internal b32 _vku_state_writes(gpu_texture_state_translation t)
{
	return (t.access & (VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
	                  | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	                  | VK_ACCESS_2_SHADER_WRITE_BIT
	                  | VK_ACCESS_2_TRANSFER_WRITE_BIT)) != 0;
}

internal gpu_texture_state_translation _vku_translate_texture_state(gpu_texture_state state)
{
    gpu_texture_state_translation out = {0};

    switch (state) 
    {
        case gpu_texture_state_undefined:
            out.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            out.stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            out.access = 0;
            break;
            
        case gpu_texture_state_color_attachment:
            out.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            out.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            out.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
            break;
            
        case gpu_texture_state_depth_attachment:
            out.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            out.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            out.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case gpu_texture_state_depth_read_only:
            out.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            out.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            out.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT;
            break;

        case gpu_texture_state_vertex_read:
            out.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            out.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            out.access = VK_ACCESS_2_SHADER_READ_BIT;
            break;

        case gpu_texture_state_fragment_read:
            out.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            out.stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            out.access = VK_ACCESS_2_SHADER_READ_BIT;
            break;

        case gpu_texture_state_present:
            out.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            out.stage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            out.access = 0;
            break;
    }

    return out;
}

void vku_coherent_descriptor_buffer_write_image(
    gpu_context *gpu, 
    gpu_buffer *buffer, 
    u32 binding, 
    u32 array_index,
    VkDescriptorSetLayout layout,
    gpu_texture *texture)
{
    VkDeviceSize offset;
    gpu->device.get_descriptor_set_layout_binding_offset(gpu->device.handle, layout, binding, &offset);

    VkDeviceSize desc_size = gpu->device.pdevice->descriptor_buffer_properties.sampledImageDescriptorSize;

    gpu_texture_state_translation translation = _vku_translate_texture_state(texture->state);

    gpu->device.get_descriptor(
        gpu->device.handle, 
        (VkDescriptorGetInfoEXT[]){{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 
            .data = {
                .pSampledImage = (VkDescriptorImageInfo[]){{
                    .imageLayout = translation.layout,
                    .imageView = texture->view_handle,    
                }} 
            }
        }}, 
        desc_size, 
        (u8*)buffer->mem + offset + (array_index * desc_size)
    );
}



void vku_local_descriptor_buffer_write_image_record(
    VkCommandBuffer cmd,
    gpu_context *gpu, 
    gpu_buffer *buffer,
    gpu_buffer_arena *frame_deferred_src_arena, 
    u32 binding, 
    u32 array_index,
    VkDescriptorSetLayout layout,
    gpu_texture *texture,
    gpu_texture_state expected_state
)
{
    VkDeviceSize offset;
    gpu->device.get_descriptor_set_layout_binding_offset(gpu->device.handle, layout, binding, &offset);

    VkDeviceSize desc_size = gpu->device.pdevice->descriptor_buffer_properties.sampledImageDescriptorSize;

    gpu_texture_state_translation translation = _vku_translate_texture_state(expected_state);

    gpu_buffer *staging_buffer = gpu_buffer_arena_push(
        u8,
        .device = &gpu->device,
        .arena = frame_deferred_src_arena,
        .tag = text128_make("vku_local_descriptor_buffer_write_sampled_image_imm staging"),
        .size = desc_size
    );

    gpu->device.get_descriptor(
        gpu->device.handle, 
        (VkDescriptorGetInfoEXT[]){{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 
            .data = {
                .pSampledImage = (VkDescriptorImageInfo[]){{
                    .imageLayout = translation.layout,
                    .imageView = texture->view_handle,    
                }} 
            }
        }}, 
        desc_size, 
        (u8*)staging_buffer->mem
    );
    
    vku_cmd_copy_buffer(cmd, staging_buffer, buffer, 0, offset + (array_index * desc_size));
}


VkDescriptorBufferBindingInfoEXT vku_descriptor_buffer_binding_info_make(gpu_context *gpu, VkBuffer buffer)
{
    VkBufferDeviceAddressInfo desc_addr_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer 
    };        

    VkDeviceAddress desc_arena_address = vkGetBufferDeviceAddress(gpu->device.handle, &desc_addr_info);

    return (VkDescriptorBufferBindingInfoEXT){
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
        .address = desc_arena_address,     
        .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
        .pNext = NULL,
    };     
}


void vku_cmd_textures_transition_state_(VkCommandBuffer cmd, arena *temp_mem, u32 transition_count, gpu_texture_transition *transitions)
{
    VkImageMemoryBarrier2 *barriers = arena_ppush_arr(temp_mem, VkImageMemoryBarrier2, transition_count);

    u32 current_barrier = 0;

    for each_index(i, transition_count)
    {
        gpu_texture_state_translation src_translation = _vku_translate_texture_state(transitions[i].texture->state);
        gpu_texture_state_translation dst_translation = _vku_translate_texture_state(transitions[i].state);

        if (transitions[i].texture->state == transitions[i].state && !_vku_state_writes(src_translation))
        {
            continue;
        }
    
        transitions[i].texture->state = transitions[i].state;

        VkImageAspectFlags aspect_mask;

        // @todo: other formats
        if (transitions[i].texture->format == VK_FORMAT_D32_SFLOAT ) 
        {
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else 
        {
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
    
        barriers[current_barrier] = (VkImageMemoryBarrier2) {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .oldLayout = src_translation.layout,
            .newLayout = dst_translation.layout,
            .image = transitions[i].texture->handle,
            .srcAccessMask =  src_translation.access,
            .srcStageMask = src_translation.stage,
            .srcQueueFamilyIndex= VK_QUEUE_FAMILY_IGNORED,
            .dstAccessMask = dst_translation.access,
            .dstStageMask = dst_translation.stage,
            .dstQueueFamilyIndex= VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0, 
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0, 
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            }
        };    

        current_barrier++;
    }

    VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = current_barrier,
        .pImageMemoryBarriers = barriers
    };

    vkCmdPipelineBarrier2(cmd, &dep_info);        
}

void vku_cmd_begin_rendering_(vku_begin_rendering_params params)
{
    u32 transition_count = params.color_attachment_count + (params.depth_attachment ? 1 : 0);
    gpu_texture_transition *transitions = arena_ppush_arr(params.temp_mem, gpu_texture_transition, transition_count);

    for each_index(i, params.color_attachment_count)    
    {
        transitions[i].texture = params.color_attachments[i];
        transitions[i].state = gpu_texture_state_color_attachment;
    }

    if(params.depth_attachment)
    {
        transitions[transition_count - 1].texture = params.depth_attachment;
        transitions[transition_count - 1].state = params.depth_read_only ? gpu_texture_state_depth_read_only : gpu_texture_state_depth_attachment;
    }

    vku_cmd_textures_transition_state_(
        params.cmd, 
        params.temp_mem, 
        transition_count, 
        transitions
    );

    VkRenderingAttachmentInfo *colors = arena_ppush_arr(params.temp_mem, VkRenderingAttachmentInfo, params.color_attachment_count);
     
    for each_index(i, params.color_attachment_count)    
    {
        colors[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colors[i].imageView = params.color_attachments[i]->view_handle;
        colors[i].imageLayout = _vku_translate_texture_state(params.color_attachments[i]->state).layout;
        colors[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colors[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colors[i].clearValue.color = params.clear_color;
    }

    VkRenderingAttachmentInfo depth = {0};
    if (params.depth_attachment) 
    {
        depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth.imageView = params.depth_attachment->view_handle;
        depth.imageLayout = _vku_translate_texture_state(params.depth_attachment->state).layout;
        depth.loadOp = params.depth_load_op;
        depth.storeOp = params.depth_store_op;
        depth.clearValue.depthStencil = (VkClearDepthStencilValue){ 1.0f, 0.0f };
    }

    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = {0,0}, .extent = params.extent },
        .layerCount = 1,
        .colorAttachmentCount = params.color_attachment_count,
        .pColorAttachments = colors,
        .pDepthAttachment = params.depth_attachment ? &depth : NULL
    };

    vkCmdBeginRendering(params.cmd, &render_info);
}

void vku_cmd_set_viewport_and_scissors(VkCommandBuffer cmd, VkExtent2D extent)
{
    vkCmdSetViewport(cmd, 0, 1, (VkViewport[]){{
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    }});
    vkCmdSetScissor(cmd, 0, 1, (VkRect2D[]){{
        .offset = {0, 0},
        .extent = extent,
    }});
}
