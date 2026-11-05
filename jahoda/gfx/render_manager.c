#include "render_manager.h"
#include <cgltf.h>
#include <stb_image.h>

typedef struct
{
    VkDeviceAddress mesh_bounding_sphere_buffer;           
    VkDeviceAddress instance_mesh_buffer;
    VkDeviceAddress instance_transform_buffer;
    VkDeviceAddress indirect_buffer;
    VkDeviceAddress visibility_ids;
} dispatch_buffer;

render_manager render_manager_make_(render_manager_params params)
{
    render_manager out = {.name = params.name};

    out.model_capacity = params.model_capacity;
    out.mesh_capacity = params.mesh_capacity;
    out.texutre_capacity = params.texture_capacity;

    out.local_ssbo_arena = gpu_buffer_arena_make(
        .mem = params.mem,
        .name = text128_make("static_arenas.local_ssbo_arena"),
        .device =  &params.gpu->device,      
        .capacity = sizeof(vec4_f32) * params.mesh_capacity,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .flags = 0,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .sharing_mode = VK_SHARING_MODE_EXCLUSIVE
    );      

    out.local_rgb8_arena = gpu_texture_arena_make(
        .mem = params.mem,
        .name = text128_make("%s.local_rgb8_arena", params.name.data),
		.device = &params.gpu->device, 		
		.capacity = params.rgb8_memory, 
		.format = VK_FORMAT_R8G8B8A8_UNORM,		 
		.samples = VK_SAMPLE_COUNT_1_BIT, 
		.flags = 0, 
		.tiling = VK_IMAGE_TILING_OPTIMAL, 
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		.sharing_mode = VK_SHARING_MODE_EXCLUSIVE
	); 	

    out.models = arena_ppush_arr(params.mem, render_model, params.model_capacity);
    out.meshes = arena_ppush_arr(params.mem, render_mesh, params.mesh_capacity);

    uz vertex_count = params.vertex_memory / render_vertex_size;
    uz index_count = params.index_memory / sizeof(u32);

    out.local_vertex_arena = gpu_buffer_arena_make(
        .mem = params.mem,
		.name = text128_make("%s.local_vertex_arena", params.name.data),
		.device = &params.gpu->device, 		
		.capacity = params.vertex_memory, 
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.flags = 0,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.sharing_mode = VK_SHARING_MODE_EXCLUSIVE
	);

    out.local_index_arena = gpu_buffer_arena_make(
        .mem = params.mem,
		.name = text128_make("%s.local_index_arena", params.name.data),
		.device = &params.gpu->device, 		
		.capacity = params.index_memory, 
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.flags = 0,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.sharing_mode = VK_SHARING_MODE_EXCLUSIVE
	);

    #define X(name_, type, vk_type)\
    out.geometry.name_##s = gpu_buffer_arena_push(\
        type,\
        .tag = text128_make("[%s] %s", params.name.data, #name_ "s"),\
        .arena = out.local_vertex_arena, \
        .device = &params.gpu->device, \
        .size = vertex_count\
    );
    render_vertex_attribute_list
    #undef X

    out.geometry.indices = gpu_buffer_arena_push(
        u32, 
        .tag = text128_make("[%s] indices", params.name.data),
        .arena = out.local_index_arena, 
        .device = &params.gpu->device, 
        .size = index_count
    );

    out.mesh_bounding_spheres = gpu_buffer_arena_push(
        render_mesh_bounding_sphere, 
        .tag = text128_make("[%%s] mesh_bounding_spheres", params.name.data),
        .arena = out.local_ssbo_arena, 
        .device = &params.gpu->device, 
        .size = params.mesh_capacity
    );

    return out;
}


internal render_mesh_id render_world_load_cgltf_primitive(
    render_manager *manager, 
    gpu_context *gpu, 
    gpu_buffer_arena *temp_coherent_transfer_source,
    text64 primitive_name, 
    cgltf_primitive *gltf_primitive, 
    arena *mem, 
    arena *temp_mem)
{
    gpu_buffer_arena_scratch scratch = gpu_buffer_arena_scratch_make(temp_coherent_transfer_source);

    cgltf_accessor *position_accessor = NULL;
    cgltf_accessor *normal_accessor = NULL;
    cgltf_accessor *tangent_accessor = NULL;
    cgltf_accessor *uv_accessor = NULL;

    for (cgltf_size i = 0; i < gltf_primitive->attributes_count; ++i) 
    {
        if (gltf_primitive->attributes[i].type == cgltf_attribute_type_position) 
        {
            position_accessor = gltf_primitive->attributes[i].data;
        } 
        else if (gltf_primitive->attributes[i].type == cgltf_attribute_type_normal) 
        {
            normal_accessor = gltf_primitive->attributes[i].data;
        }
        else if (gltf_primitive->attributes[i].type == cgltf_attribute_type_tangent) 
        {
            tangent_accessor = gltf_primitive->attributes[i].data;
        }
        else if (gltf_primitive->attributes[i].type == cgltf_attribute_type_texcoord) 
        {
            uv_accessor = gltf_primitive->attributes[i].data;
        }
    }

    verifyl(render_domain_load_imm, position_accessor != NULL, "gltf primitive has no positions");
    
    u32 vertex_count = position_accessor->count;
    u32 index_count = gltf_primitive->indices ? gltf_primitive->indices->count : vertex_count;

    gpu_buffer *positions_staging = gpu_buffer_arena_push(vec3_f32,
        .tag = text128_make("[%s] positions_staging_buffer", primitive_name.data),            
        .arena = temp_coherent_transfer_source, 
        .device = &gpu->device,              
        .size = vertex_count
    );

    gpu_buffer *normals_staging = gpu_buffer_arena_push(vec3_f32,
        .tag = text128_make("[%s] normals_staging_buffer", primitive_name.data),            
        .arena = temp_coherent_transfer_source, 
        .device = &gpu->device,              
        .size = vertex_count
    );

    gpu_buffer *indices_staging = gpu_buffer_arena_push(
        u32,
        .tag = text128_make("[%s] indices_staging", primitive_name.data),
        .arena = temp_coherent_transfer_source,
        .device = &gpu->device,
        .size = index_count
    );  

    vec3_f32 *positions = positions_staging->mem;
    vec3_f32 *normals = normals_staging->mem;
    u32 *indices = indices_staging->mem;

    for (u32 i = 0; i < vertex_count; ++i) 
    {        
        cgltf_accessor_read_float(position_accessor, i, &positions[i].x, 3);
    
        if (normal_accessor) 
        {
            cgltf_accessor_read_float(normal_accessor, i, &normals[i].x, 3);
        } 
        else 
        {    
            normals[i] = (vec3_f32){0, 1, 0}; 
        }
    }

    if (gltf_primitive->indices) 
    {
        for (u32 i = 0; i < index_count; ++i) 
        {
            indices[i] = cgltf_accessor_read_index(gltf_primitive->indices, i);
        }
    } 
    else 
    {       
        for (u32 i = 0; i < index_count; ++i) 
        {
            indices[i] = i;
        }
    }

    VkCommandBuffer cmd = gpu_context_grab_graphics(gpu);
    
    vku_cmd_copy_buffer(cmd, positions_staging, manager->geometry.positions, 0, manager->geometry.vertex_current * sizeof(vec3_f32));
    vku_cmd_copy_buffer(cmd, normals_staging, manager->geometry.normals, 0, manager->geometry.vertex_current * sizeof(vec3_f32));
    vku_cmd_copy_buffer(cmd, indices_staging, manager->geometry.indices, 0, manager->geometry.index_current * sizeof(u32));

    render_mesh_id id = manager->mesh_current++;    

    dbg_verify(id < manager->mesh_capacity);

    manager->meshes[id].indices_count = index_count;
    manager->meshes[id].indices_offset =  manager->geometry.index_current;
    manager->meshes[id].vertex_offset = manager->geometry.vertex_current;
    manager->geometry.vertex_current += vertex_count;
    manager->geometry.index_current += index_count;

    vec3_f32 min_bounds = positions[0];
    vec3_f32 max_bounds = positions[0];
    
    for (u32 i = 1; i < vertex_count; ++i) 
    {
        if (positions[i].x < min_bounds.x) min_bounds.x = positions[i].x;
        if (positions[i].y < min_bounds.y) min_bounds.y = positions[i].y;
        if (positions[i].z < min_bounds.z) min_bounds.z = positions[i].z;

        if (positions[i].x > max_bounds.x) max_bounds.x = positions[i].x;
        if (positions[i].y > max_bounds.y) max_bounds.y = positions[i].y;
        if (positions[i].z > max_bounds.z) max_bounds.z = positions[i].z;
    }

    vec3_f32 center = {
        (min_bounds.x + max_bounds.x) * 0.5f,
        (min_bounds.y + max_bounds.y) * 0.5f,
        (min_bounds.z + max_bounds.z) * 0.5f
    };

    f32 max_radius_sq = 0.0f;
    for (u32 i = 0; i < vertex_count; ++i) 
    {
        f32 dx = positions[i].x - center.x;
        f32 dy = positions[i].y - center.y;
        f32 dz = positions[i].z - center.z;
        
        f32 dist_sq = (dx * dx) + (dy * dy) + (dz * dz);
        if (dist_sq > max_radius_sq) 
        {
            max_radius_sq = dist_sq;
        }
    }

    vec4_f32 bounding_sphere = {
        center.x, 
        center.y, 
        center.z, 
        sqrtf(max_radius_sq)
    };

    gpu_buffer *mesh_boudning_sphere_stagin = gpu_buffer_arena_push(
        render_mesh_bounding_sphere,
        .tag = text128_make("[%s] mesh_boudning_sphere_stagin", primitive_name.data),
        .arena = temp_coherent_transfer_source,
        .device = &gpu->device,
        .size = 1,
    );
    
    *(vec4_f32*)(mesh_boudning_sphere_stagin->mem) = bounding_sphere;
    
    vku_cmd_copy_buffer(cmd, mesh_boudning_sphere_stagin, manager->mesh_bounding_spheres, 0, sizeof(vec4_f32) * id);                

    gpu_context_commit_graphics(gpu, cmd);

    gpu_context_immidiate_graphics(gpu);

    gpu_buffer_arena_scratch_pop(scratch, &gpu->device);

    return id;
}

internal void render_world_parse_gltf_node(
    render_manager *manager, 
    gpu_context *gpu,
    gpu_buffer_arena *temp_coherent_transfer_source,
    cgltf_data *data,
    const char *display_name,
    cgltf_node *node, 
    u32 scene_material_offset,
    arena *mem,
    arena *temp_mem,
    render_primitive_node *out_node
)
{
    if (node->mesh) 
    {
        out_node->primitives = arena_ppush_arr(mem, render_primitive, node->mesh->primitives_count);

        for each_index(primitive_id, node->mesh->primitives_count)
        {
            cgltf_primitive *primitive = &node->mesh->primitives[primitive_id];
            
            // render_domain_id domain_id = router->opaque_geometry_domain_id;
            // if (primitive->material && primitive->material->alpha_mode == cgltf_alpha_mode_blend) 
            // {
            //     domain_id = router->transparent_geometry_domain_id;
            // }
            // model->primitive_count = gltf_node->mesh->primitives_count;        
            // model->primitives = arena_ppush_arr(mem, render_primitive, gltf_node->mesh->primitives_count);

            u32 primitive_local_id = out_node->primitive_count++;

            out_node->primitives[primitive_local_id] = (render_primitive){
                .material_id = scene_material_offset + cgltf_material_index(data, primitive->material), 
                .mesh_id = render_world_load_cgltf_primitive(manager, gpu, temp_coherent_transfer_source, text64_make("%s.node[%p][%u]", display_name, node, primitive_id), primitive, mem, temp_mem),
            };
        }
    }

    out_node->nodes = arena_ppush_arr(mem, render_primitive_node, node->children_count);

    for each_index(child_idx, node->children_count)
    {    
        cgltf_node_transform_local(node, (f32*)&out_node->nodes[child_idx].local_transform);

        render_world_parse_gltf_node(
            manager, 
            gpu,
            temp_coherent_transfer_source,
            data, 
            display_name,
            node->children[child_idx],
            scene_material_offset, 
            mem,
            temp_mem, 
            out_node->nodes + child_idx
        );

        out_node->node_count++;
    }
}


internal void *proxy_cgltf_alloc_func(void* user, cgltf_size size)
{
    arena *mem = (arena*)user;
    return arena_push_bytes(mem, size, 1, false);
}

internal void proxy_cgltf_free_func(void* user, void* ptr) {}


internal void gltf_load_textures(u32 texture_count, cgltf_texture *textures, str8 gltf_path, arena *temp_mem)
{
    for each_index(texture_id, texture_count)
    {
        cgltf_image *image = textures[texture_id].image;
        int channels;
        int width;
        int height;
    
        if (image->buffer_view)
        {
            u8 *bytes = image->buffer_view->buffer->data + image->buffer_view->offset;
            u8 *data = stbi_load_from_memory(bytes, image->buffer_view->size, &width, &height, &channels, 4);
        }    
        else if (image->uri)
        {
            uz uri_len = strlen(image->uri);
            char *path = arena_ppush_arr(temp_mem, char, gltf_path.size + uri_len + 1);
            memcpy(path, gltf_path.data, gltf_path.size);
            memcpy(path + gltf_path.size, image->uri, uri_len);
            u8 *data = stbi_load(path, &width, &height, &channels, 4);
        }
        else {
            err();
        }
    }
}

render_texture_id render_world_load_cgltf_texture(
    render_manager *manager, 
    gpu_context *gpu,
    cgltf_data *data, 
    cgltf_texture_view *view,
    gpu_buffer_arena *temp_coherent_transfer_source,
    arena *mem, 
    arena *temp_mem, 
    str8 gltf_path)
{ 
}

render_model_id render_manager_load_gltf(
    render_manager *manager, 
    gpu_context *gpu,
    gpu_buffer_arena *temp_coherent_transfer_source,
    arena *mem, 
    arena *temp_mem, 
    str8 gltf_path, 
    u32 scene_id)
{
    scratch scratch_mem = scratch_begin(temp_mem);
    
    cgltf_options options = {
        .memory = {
            .alloc_func = &proxy_cgltf_alloc_func,
            .free_func = &proxy_cgltf_free_func,
            .user_data = (void*)(temp_mem)
        }
    };

    cgltf_data *data = {0};
    cgltf_result result = cgltf_parse_file(&options, gltf_path.data, &data);
    verifyl(render_domain_load_imm, result == cgltf_result_success, "failed to parse gltf: %s\n", gltf_path.data);
    
    result = cgltf_load_buffers(&options, data, gltf_path.data);
    verifyl(render_domain_load_imm, result == cgltf_result_success, "failed to load gltf buffers: %s\n", gltf_path.data);
    
    // gltf_load_textures(data->textures, data->textures_count, gltf_path, temp_mem, int *w, int *h)

    cgltf_scene *gltf_scene = scene_id == render_scene_default ? data->scene : data->scenes + scene_id; 

    
    verify(manager->model_current != manager->model_capacity);
    render_model_id out = manager->model_current++;
    render_model *model = manager->models + out;
    
    model->nodes = arena_ppush_arr(mem, render_primitive_node, gltf_scene->nodes_count);
    
    for each_index(scene_node_id, gltf_scene->nodes_count)
    {
        cgltf_node *gltf_node = gltf_scene->nodes[scene_node_id]; 
        
        cgltf_node_transform_local(gltf_node, (f32*)&model->nodes[scene_node_id].local_transform);
        
        render_world_parse_gltf_node(
            manager, 
            gpu,
            temp_coherent_transfer_source,
            data,
            gltf_scene->name ? gltf_scene->name : gltf_path.data,
            gltf_node, 
            0,
            mem,
            temp_mem, 
            model->nodes + scene_node_id
        );        

        model->node_count++;
    }
    
    scratch_end(scratch_mem);

    return out;   
}

render_batch render_batch_make_(render_batch_params params)
{
    render_batch out = (render_batch){
        .instance_counts = arena_ppush_arr(params.mem, u32, params.manager->mesh_capacity),
        .mesh_id_buffer = gpu_buffer_arena_push(
            render_mesh_id,
            .arena = params.any_ssbo_arena,
            .tag = params.tag,
            .device = &params.gpu->device,
            .size = params.instance_count,
        ),
        .transform_buffer = gpu_buffer_arena_push(
            render_transform,
            .arena = params.any_ssbo_arena,
            .tag = params.tag,
            .device = &params.gpu->device,
            .size = params.instance_count,
        ),
        .mesh_count = params.manager->mesh_current,
        .instance_capacity = params.instance_count
    };

    return out;

}

render_batch *render_batch_reset(render_batch *batch)
{
    memset(batch->instance_counts, 0, sizeof(u32) * batch->mesh_count);
    return batch;
}


render_command render_command_make_(render_command_params params)
{
    render_command out = {
        .cpu_indirect_commands = arena_ppush_arr(params.mem, VkDrawIndexedIndirectCommand, params.manager->mesh_capacity),            
        .indirect_commands_buffer = gpu_buffer_arena_push(
            VkDrawIndexedIndirectCommand, 
            .arena = params.local_indirect_command_arena,
            .device = &params.gpu->device,
            .tag = params.tag,
            .size = params.manager->mesh_capacity
        ),
        .dispatch_buffer = gpu_buffer_arena_push(
            dispatch_buffer, 
            .arena = params.coherent_ssbo_arena,
            .device = &params.gpu->device,
            .tag = params.tag,
            .size = 1
        ),
        .visible_id_buffer = gpu_buffer_arena_push(
            u32, 
            .arena = params.coherent_ssbo_arena,
            .device = &params.gpu->device,
            .tag = params.tag,
            .size = params.batch->instance_capacity
        ),
        .indirect_command_count = params.manager->mesh_current,
        .batch = params.batch
    };

    for each_index(i, params.manager->mesh_current)
    {
        out.cpu_indirect_commands[i].instanceCount = 0;
        out.cpu_indirect_commands[i].firstIndex = params.manager->meshes[i].indices_offset;
        out.cpu_indirect_commands[i].indexCount = params.manager->meshes[i].indices_count;
        out.cpu_indirect_commands[i].vertexOffset = params.manager->meshes[i].vertex_offset;
    }

    return out;
}

typedef struct
{
    vec4_f32 planes[6];
    VkDeviceAddress dispatch_buffer;
    u32 total_instances;
} render_command_fill_pass_push_constants;
static_assert(sizeof(render_command_fill_pass_push_constants) <= 128);

void render_command_fill_pass_record(
    VkCommandBuffer cmd, 
    render_command_fill_pass *pass, 
    arena *temp_mem, 
    gpu_context *gpu, 
    render_manager *manager,
    u32 command_count,
    render_command (*commands)[frames_in_flight],
    camera *cam)
{
    if(!pass->compute_state.pipeline)
    {
        pass->compute_state = compute_state_make(
            .gpu = gpu,
            .temp_mem = temp_mem,
            .shader_path = str8_lit("assets/shaders/frustum_cull.comp.spv"),
            .compute_state_push_constants(render_command_fill_pass_push_constants),
        );
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pass->compute_state.pipeline);

    u32 *instance_counts = arena_ppush_arr(temp_mem, u32, command_count);

    for each_index(cmd_id, command_count)
    {
        render_command *command = commands[cmd_id] + gpu->frame_index;
        render_batch *batch = command->batch;
        
        for each_index(i, command->indirect_command_count)
        {   
            command->cpu_indirect_commands[i].firstInstance = instance_counts[cmd_id];
            instance_counts[cmd_id] += batch->instance_counts[i];
        }
        
        uz update_size = sizeof(VkDrawIndexedIndirectCommand) * command->indirect_command_count;
        dbg_verify(update_size <= Kb(64));
            
        if(update_size)
        {
            vkCmdUpdateBuffer(cmd, command->indirect_commands_buffer->handle, 0, update_size, command->cpu_indirect_commands);
        }
        
    }
    
    vku_cmd_memory_barrier(
        cmd,

        VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, 
        VK_ACCESS_2_TRANSFER_WRITE_BIT,

        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT
    );

    for each_index(cmd_id, command_count)
    {
        render_command *command = commands[cmd_id] + gpu->frame_index;
        render_batch *batch = command->batch;
        
        mat4_f32 vp = cam->proj_view; 
    
        render_command_fill_pass_push_constants pc = {        
            .planes[0] = (vec4_f32){ vp.x.w + vp.x.x, vp.y.w + vp.y.x, vp.z.w + vp.z.x, vp.w.w + vp.w.x },
            .planes[1] = (vec4_f32){ vp.x.w - vp.x.x, vp.y.w - vp.y.x, vp.z.w - vp.z.x, vp.w.w - vp.w.x },
            .planes[2] = (vec4_f32){ vp.x.w + vp.x.y, vp.y.w + vp.y.y, vp.z.w + vp.z.y, vp.w.w + vp.w.y },
            .planes[3] = (vec4_f32){ vp.x.w - vp.x.y, vp.y.w - vp.y.y, vp.z.w - vp.z.y, vp.w.w - vp.w.y },
            .planes[4] = (vec4_f32){ vp.x.z, vp.y.z, vp.z.z, vp.w.z },
            .planes[5] = (vec4_f32){ vp.x.w - vp.x.z, vp.y.w - vp.y.z, vp.z.w - vp.z.z, vp.w.w - vp.w.z },
        };
    
        for each_index(i, 6) 
        {
            f32 length = sqrtf(pc.planes[i].x * pc.planes[i].x + pc.planes[i].y * pc.planes[i].y + pc.planes[i].z * pc.planes[i].z);
            f32 inv_len = 1.0f / length;
            pc.planes[i].x *= inv_len; pc.planes[i].y *= inv_len; pc.planes[i].z *= inv_len; pc.planes[i].w *= inv_len;
        }
    
        dispatch_buffer *db = (dispatch_buffer*)command->dispatch_buffer->mem;

        *db = (dispatch_buffer){
            .mesh_bounding_sphere_buffer = manager->mesh_bounding_spheres->device_address,
            .instance_mesh_buffer       = batch->mesh_id_buffer->device_address,
            .instance_transform_buffer  = batch->transform_buffer->device_address,
            .indirect_buffer            = command->indirect_commands_buffer->device_address,
            .visibility_ids             = command->visible_id_buffer->device_address,
        };
    
        pc.dispatch_buffer = command->dispatch_buffer->device_address;
        pc.total_instances = instance_counts[cmd_id];
    
        compute_state_cmd_push_constants(
            cmd, 
            &pass->compute_state,
            &pc
        );
        
        if (instance_counts[cmd_id] > 0) 
        {
            u32 group_count_x = (instance_counts[cmd_id] + 63) / 64;                
            vkCmdDispatch(cmd, group_count_x, 1, 1);
        }    
    }

    vku_cmd_memory_barrier(
        cmd,

        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
        VK_ACCESS_2_SHADER_WRITE_BIT,

        VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
        VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT
    );
}

void render_batch_fill_pass_release(
    VkCommandBuffer cmd, 
    render_command_fill_pass *pass, 
    gpu_context *gpu)
{
    if(!pass->compute_state.pipeline)
    {
        compute_state_release(&pass->compute_state, gpu);
    }
}