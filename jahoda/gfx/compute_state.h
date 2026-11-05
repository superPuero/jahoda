#include "gpu_context.h"

typedef struct
{
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDeviceSize descriptor_layout_size;
	VkDescriptorSetLayout descriptor_layout;
} compute_state;

typedef struct
{
    gpu_context *gpu;

    str8 shader_path;

    arena *temp_mem;

    u32 push_constants_size;
    VkShaderStageFlags push_constant_stages;
    
    u32 descriptor_layout_count;
    u32 descriptor_binding_count;
    VkDescriptorSetLayoutBinding *descriptor_bindings;
} compute_state_params;

#define compute_state_descriptor_bindings(...) .descriptor_binding_count = arrlen((VkDescriptorSetLayoutBinding[]){__VA_ARGS__}), .descriptor_bindings = (VkDescriptorSetLayoutBinding[]){__VA_ARGS__}

#define compute_state_push_constants(type) push_constants_size = sizeof(type)

#define compute_state_cmd_push_constants(cmd, rs, pc)\
static_assert(sizeof(*pc) <= 128);\
vkCmdPushConstants(\
    cmd,\
    (rs)->pipeline_layout,\
    VK_SHADER_STAGE_COMPUTE_BIT,\
    0,\
    sizeof(*pc),\
    pc\
)

#define compute_state_make(...) compute_state_make_((compute_state_params){__VA_ARGS__})
compute_state compute_state_make_(compute_state_params params);
void compute_state_release(compute_state *state, gpu_context *gpu);
