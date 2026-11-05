#ifndef jahoda_gpu_utils
#define jahoda_gpu_utils

#include "gpu_context.h"
#include "gpu_shader.h"

#define vk_residency_local VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
#define vk_residency_coherent VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT

void vku_coherent_descriptor_buffer_write_image(
    gpu_context *gpu, 
    gpu_buffer *buffer, 
    u32 binding, 
    u32 array_index, 
    VkDescriptorSetLayout descriptor_set_layout, 
    gpu_texture *texture);

void vku_local_descriptor_buffer_write_image_record(
    VkCommandBuffer cmd, 
    gpu_context *gpu, 
    gpu_buffer *buffer, 
    gpu_buffer_arena *frame_deferred_src_arena, 
    u32 binding, 
    u32 array_index, 
    VkDescriptorSetLayout layout, 
    gpu_texture *texture, 
    gpu_texture_state expected_state);

static inline VkExtent2D vku_extent3d_as_2d(VkExtent3D extent)
{
    return (VkExtent2D){extent.width, extent.height};
}

VkDescriptorBufferBindingInfoEXT vku_descriptor_buffer_binding_info_make(gpu_context *gpu, VkBuffer buffer);

typedef struct
{
    gpu_texture *texture;
    gpu_texture_state state;
} gpu_texture_transition;

#define vku_cmd_textures_transition_state(cmd, temp_mem,...) vku_cmd_textures_transition_state_(cmd, temp_mem, arrlen((gpu_texture_transition[]){__VA_ARGS__}), (gpu_texture_transition[]){__VA_ARGS__});
void vku_cmd_textures_transition_state_(VkCommandBuffer cmd, arena *temp_mem, u32 transition_count, gpu_texture_transition *transitions);

// void vku_cmd_copy_buffer(VkCommandBuffer cmd, gpu_buffer *src, gpu_buffer *dst);
void vku_cmd_set_viewport_and_scissors(VkCommandBuffer cmd, VkExtent2D extent);

typedef struct
{
    VkCommandBuffer cmd;
    arena *temp_mem;
    VkExtent2D extent;
    u32 color_attachment_count;
    gpu_texture **color_attachments;
    gpu_texture *depth_attachment;
    b8 depth_read_only;
    VkAttachmentLoadOp depth_load_op;
    VkAttachmentStoreOp depth_store_op;
    VkClearColorValue clear_color;
} vku_begin_rendering_params;
#define vku_begin_render_color_attachments(...) color_attachment_count = arrlen((gpu_texture*[]){__VA_ARGS__}), .color_attachments = (gpu_texture*[]){__VA_ARGS__}

#define vku_cmd_begin_rendering(...) vku_cmd_begin_rendering_((vku_begin_rendering_params){\
        .depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,\
        .depth_store_op = VK_ATTACHMENT_STORE_OP_NONE,\
    __VA_ARGS__})
void vku_cmd_begin_rendering_(vku_begin_rendering_params params);

static inline void vku_cmd_copy_buffer(VkCommandBuffer cmd, gpu_buffer *src, gpu_buffer *dest, u64 src_offset, u64 dest_offset)
{
    VkBufferCopy region = {
        .srcOffset = src_offset,
        .dstOffset = dest_offset,
        .size = src->size,
    };
    vkCmdCopyBuffer(cmd, src->handle, dest->handle, 1, &region);       
}

static inline void vku_cmd_memory_barrier(
    VkCommandBuffer         cmd, 
    VkPipelineStageFlags2   src_stage, 
    VkAccessFlags2          src_access, 
    VkPipelineStageFlags2   dst_stage, 
    VkAccessFlags2          dst_access)
{
    VkMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = src_stage,
        .srcAccessMask = src_access,
        .dstStageMask = dst_stage,
        .dstAccessMask = dst_access
    };
    VkDependencyInfo dep = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier
    };
    vkCmdPipelineBarrier2(cmd, &dep);
}

static inline void vku_cmd_image_barrier(
    VkCommandBuffer cmd,
    VkImageMemoryBarrier2 barrier
)
{
	VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier
	};

	vkCmdPipelineBarrier2(cmd, &dep_info);
}

typedef struct
{
    VkDeviceSize offset;
    VkBuffer buffer;
} vku_vertex_binding;

typedef struct
{
    VkCommandBuffer cmd;
    u32 offset;
    u32 binding_count;
    vku_vertex_binding *bindings;
    VkBuffer index_buffer;
} vku_bind_vertex_buffers_params;

#define vku_vertex_buffers(...) binding_count = arrlen((vku_vertex_binding[]){__VA_ARGS__}), .bindings = (vku_vertex_binding[]){__VA_ARGS__}
#define vku_cmd_bind_buffers(...) vku_cmd_bind_vertex_buffers_((vku_bind_vertex_buffers_params){__VA_ARGS__})

static inline void vku_cmd_mdi(VkCommandBuffer cmd, gpu_buffer *buffer, u32 count)
{
    vkCmdDrawIndexedIndirect(
        cmd, 
        buffer->handle, 
        0, 
        count, 
        sizeof(VkDrawIndexedIndirectCommand)
    );
}

static inline void vku_cmd_bind_vertex_buffers_(vku_bind_vertex_buffers_params params)
{
    VkBuffer buffers[8] = {0};
    VkDeviceSize offsets[8] = {0};

    for each_index(i, params.binding_count)
    {
        buffers[i] = params.bindings[i].buffer;
        offsets[i] = params.bindings[i].offset;
    }

    vkCmdBindVertexBuffers(
        params.cmd, 
        params.offset, 
        params.binding_count, 
        buffers, 
        offsets
    );
	
    vkCmdBindIndexBuffer(
        params.cmd,  
        params.index_buffer,  
        0,
        VK_INDEX_TYPE_UINT32
    );
}


#endif