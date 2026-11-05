#ifndef jahoda_render_manager
#define jahoda_render_manager

#include <jahoda/core/math.h>
#include "gpu_context.h"
#include "gpu_arena.h"
#include "vertex.h"
#include "compute_state.h"
#include "gpu_utils.h"
#include "camera.h"

typedef struct alignas(16)
{
    mat4_f32 mat;
} render_transform;

typedef struct alignas(16)
{
    vec4_f32 value;
} render_mesh_bounding_sphere;

typedef struct 
{
    u32 vertex_current;
    #define X(name, type, format) gpu_buffer *name##s;
    render_vertex_attribute_list
    #undef X

    u32 index_current;
    gpu_buffer *indices;
} render_geometry;

typedef u32 render_mesh_id;
typedef u32 render_material_id;
typedef u32 render_texture_id;
typedef u32 render_model_id;

typedef struct
{
    vec4_f32 base_color;
    f32 metalic;
    f32 roughness;
    f32 ao;

    u32 normal_texture_id;
    u32 base_color_texture_id;
    u32 metalic_roughness_texture_id;
    u32 ao_texture_id;
} render_material;

typedef struct render_primitive render_primitive;
typedef struct render_primitive_node render_primitive_node;

typedef struct 
{
    u32 vertex_offset;
    u32 indices_offset;
    u32 indices_count;
} render_mesh;

struct render_primitive
{
    render_mesh_id mesh_id;   
    render_material_id material_id;
};

struct render_primitive_node
{
    u32 primitive_count;
    render_primitive *primitives;
    
    mat4 local_transform;
    
    u32 node_count;
    render_primitive_node *nodes;
};

typedef struct
{
    u32 node_count;
    render_primitive_node *nodes;
} render_model;

typedef struct
{        
    text64 name;
    
    render_geometry geometry;

    gpu_buffer_arena *local_vertex_arena;
    gpu_buffer_arena *local_index_arena;
    gpu_buffer_arena *local_ssbo_arena;       

    // @todo: add srgb support
    gpu_texture_arena *local_rgb8_arena;
    gpu_buffer *local_descriptor_buffer;
    VkDescriptorSetLayout descriptor_layour;
    VkDescriptorBufferBindingInfoEXT descriptor_binding_info;
    gpu_buffer *mesh_bounding_spheres;    

    u32 material_current;
    u32 material_capacity;
    gpu_buffer *material_buffer;    

    u32 texutre_current;
    u32 texutre_capacity;
    render_model **textures;

    u32 model_current;
    u32 model_capacity;
    render_model *models;

    u32 mesh_current;
    u32 mesh_capacity;
    render_mesh *meshes;
} render_manager;

typedef struct
{
    text64 name;

    gpu_context *gpu;
    arena *mem;

    u32 rgb8_memory;
    u32 vertex_memory;
    u32 index_memory;

    u32 texture_capacity;
    u32 model_capacity;
    u32 mesh_capacity;
} render_manager_params;


render_manager render_manager_make_(render_manager_params params);
#define render_manager_make(...) render_manager_make_((render_manager_params){__VA_ARGS__})

#define render_scene_default ~((u32)0)
render_model_id render_manager_load_gltf(
    render_manager *manager, 
    gpu_context *gpu,
    gpu_buffer_arena *temp_coherent_transfer_source,
    arena *mem, 
    arena *temp_mem, 
    str8 gltf_path, 
    u32 scene_id);

typedef struct render_batch render_batch;

typedef struct
{
    gpu_buffer *indirect_commands_buffer; 
    gpu_buffer *visible_id_buffer;    
    gpu_buffer *dispatch_buffer;
    VkDrawIndexedIndirectCommand *cpu_indirect_commands;
    u32 indirect_command_count;
    render_batch *batch;
} render_command;

struct render_batch
{
    gpu_buffer *transform_buffer;
    gpu_buffer *mesh_id_buffer;
    u32 *instance_counts;
    u32 mesh_count;
    u32 instance_capacity;
    u32 view_count;
};

typedef struct
{
    render_manager *manager;
    gpu_buffer_arena *any_ssbo_arena;
    gpu_context *gpu;
    arena *mem;
    text128 tag;
    u32 instance_count;
} render_batch_params;

render_batch render_batch_make_(render_batch_params params);
#define render_batch_make(...) render_batch_make_((render_batch_params){__VA_ARGS__});

render_batch *render_batch_reset(render_batch *batch);

typedef struct
{
    text128 tag;
    render_manager *manager;
    render_batch *batch;
    arena *mem;
    gpu_context *gpu;
    gpu_buffer_arena *local_indirect_command_arena;
    gpu_buffer_arena *coherent_ssbo_arena;
    gpu_buffer_arena *local_ssbo_arena;
} render_command_params;

render_command render_command_make_(render_command_params params);
#define render_command_make(...) render_command_make_((render_command_params){__VA_ARGS__});

typedef struct
{
    compute_state compute_state;
} render_command_fill_pass;

void render_command_fill_pass_record(
    VkCommandBuffer cmd, 
    render_command_fill_pass *pass, 
    arena *temp_mem, 
    gpu_context *gpu, 
    render_manager *manager,
    u32 command_count,
    render_command (*commands)[frames_in_flight],
    camera *cam);

void render_batch_fill_pass_release(VkCommandBuffer cmd, render_command_fill_pass *pass, gpu_context *gpu);

#endif