#ifndef jahoda_render_state
#define jahoda_render_state

#include "gpu_context.h"
#include "vertex.h"

typedef struct
{
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDeviceSize descriptor_layout_size;
	VkDescriptorSetLayout descriptor_layout;
    VkShaderStageFlags push_constant_stages;
} render_state;

#define render_state_cmd_push_constants(cmd, rs, pc)\
static_assert(sizeof(*pc) <= 128);\
vkCmdPushConstants(\
    cmd,\
    (rs)->pipeline_layout,\
    (rs)->push_constant_stages,\
    0,\
    sizeof(*pc),\
    pc\
)

typedef struct
{
    uint32_t    location;
    uint32_t    stride;
    VkFormat    format;
} render_state_vertex_binding;

static render_state_vertex_binding render_vertex_binding[] = 
{
    #define X(name, type, f) { .location = render_vertex_attribute_##name, .stride = sizeof(type), .format = f },
    render_vertex_attribute_list
    #undef X
};

typedef struct
{
    gpu_context *gpu;

    str8 vertex_shader_path;
    str8 fragment_shader_path;

    arena *temp_arena;

    u32 push_constants_size;
    VkShaderStageFlags push_constant_stages;
    
    u32 descriptor_binding_count;
    VkDescriptorSetLayoutBinding *descriptor_bindings;

    u32 dynamic_state_count;
    VkDynamicState *dynamic_state;

    u32 vertex_binding_count;
    render_state_vertex_binding *vertex_bindings;
    // VkVertexInputBindingDescription *vertex_bindings;
    
    // u32 vertex_attribute_count;
    // VkVertexInputAttributeDescription *vertex_attributes;

    VkPrimitiveTopology topology; // default: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    VkPolygonMode polygon_mode;   // default: VK_POLYGON_MODE_FILL
    VkCullModeFlags cull_mode;    // default: VK_CULL_MODE_BACK_BIT

    b32 depth_test; // default: VK_FALSE
    b32 depth_write; // default: VK_FALSE
    VkCompareOp depth_compare_op; // default: VK_COMPARE_OP_LESS

    u32 color_target_count;
    VkFormat *color_target_formats;
    VkFormat depth_target_format;
} render_state_params;

#define render_state_vertex_bindings(...) vertex_binding_count = arrlen((render_state_vertex_binding[]){__VA_ARGS__}), .vertex_bindings = (render_state_vertex_binding[]){__VA_ARGS__}

#define render_state_push_constants(type, stages) push_constants_size = sizeof(type), .push_constant_stages = stages

#define render_state_color_targets(...) color_target_count = arrlen((VkFormat[]){__VA_ARGS__}), .color_target_formats = (VkFormat[]){__VA_ARGS__}
#define render_state_vertex_attributes(...) vertex_attribute_count = arrlen((VkVertexInputAttributeDescription[]){__VA_ARGS__}), .vertex_attributes = (VkVertexInputAttributeDescription[]){__VA_ARGS__}
#define render_state_descriptor_bindings(...) descriptor_binding_count = arrlen((VkDescriptorSetLayoutBinding[]){__VA_ARGS__}), .descriptor_bindings = (VkDescriptorSetLayoutBinding[]){__VA_ARGS__}
#define render_state_dynamic_state(...) dynamic_state_count = arrlen((VkDynamicState[]){__VA_ARGS__}), .dynamic_state = (VkDynamicState[]){__VA_ARGS__}

#define render_state_make(...) render_state_make_((render_state_params){\
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,\
    .polygon_mode = VK_POLYGON_MODE_FILL,\
    .cull_mode = VK_CULL_MODE_BACK_BIT,\
    .depth_test = VK_FALSE,\
    .depth_write = VK_FALSE,\
    .depth_compare_op = VK_COMPARE_OP_LESS,\
    __VA_ARGS__\
})


render_state render_state_make_(render_state_params params);
void render_state_release(render_state *state, gpu_context *gpu);


#endif 