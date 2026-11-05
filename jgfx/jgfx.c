#include <jahoda/core/core.h>
#include <jahoda/core/thread_pool.h>
#include <jahoda/gfx/window.h>
#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/gpu_profiler.h>
#include <jahoda/gfx/present_pass.h>
#include <jahoda/gfx/render_manager.h>
#include <jahoda/base/tick_info.h>
#include <jahoda/core/os.h>
#include <jahoda/core/tmap.h>

#include "transform.h"
#include "depth_pass.h"
#include "gbuffer_pass.h"
#include "deferred_light_pass.h"
#include "basic_pass.h"

#define cmd_line_args_spec jgfx

#define cmd_line_args\
    X(log, cstr, "-", "destination for logging, \"stdout\" for standart output")\
    X(obj_grid_size, u32, 25, "obj_grid_size")

#define cmd_line_flag_args\
    X(h, "help")

#include <jahoda/meta/cmd_line_args.h>

typedef struct {} dude;
typedef struct {} camera_controller;
 
typedef struct { render_mesh_id mesh_id; } static_mesh;
typedef struct { render_mesh_id mesh_id; } dynamic_mesh;

#define ent_spec basic
#define ent_max_children 128
#define ent_tables\
    X(local_transform)\
    X(global_transform)\
    X(camera_controller)\
    X(dude)\
    X(static_mesh)\
    X(dynamic_mesh)

#include <jahoda/meta/ent.h>

#define ent_add_transform(manager, id, ...)\
ent_add(manager, id, local_transform, {.scale = {1,1,1}, __VA_ARGS__});\
ent_add(manager, id, global_transform, {})\

typedef struct
{
    vec2_f32 resolution;
	f32 time;
    f32 curvature;       
    f32 scanline_weight; 
    f32 mask_strength;   
    f32 aberration;      
    f32 bloom_boost;
} crt_params;

static crt_params crt_params_default = {
	.curvature = 0.15,      // 0.0 = flat, 0.15 = heavy bubble
	.scanline_weight = 0.5, // 0.0 = off, 0.5 = dark scanlines
	.mask_strength = 0.20,  // subpixel grille intensity
	.aberration = 0.0025,   
	.bloom_boost = 1.25    
};

basic_pass_declare(crt_pass, crt_params, "assets/shaders/crt.frag.spv");

#define buffer_memory_info_entry(name_, ...)\
[name_] = { \
    .ccname = #name_, \
    __VA_ARGS__\
}

#define buffer_memory_kind_list\
    X(coherent_ssbo, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vk_residency_coherent)\
    X(coherent_src, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vk_residency_coherent)\
    X(coherent_resource_descriptor_buffer, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT, vk_residency_coherent)\
    X(local_ssbo_dst, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk_residency_local)\
    X(local_indirect_ssbo_dst, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vk_residency_local)

typedef enum
{
    #define X(name, usage, residency) buffer_memory_kind_##name,
    buffer_memory_kind_list
    #undef X
    buffer_memory_kind_count
} buffer_memory_kind;

static VkBufferUsageFlags buffer_memory_kind_usage[] = {
    #define X(name, usage, residency) usage,
    buffer_memory_kind_list
    #undef X
};

static VkBufferUsageFlags buffer_memory_kind_residency[] = {
    #define X(name, usage, residency) residency,
    buffer_memory_kind_list
    #undef X
};

static const char *buffer_memory_kind_name[] = {
    #define X(name, usage, residency) #name,
    buffer_memory_kind_list
    #undef X
};

typedef struct
{
    buffer_memory_kind kind;
    u64 capacity;
} buffer_memory_entry;

#define texture_memory_kind_list\
    X(local_sampled_r8g8b8a8, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, vk_residency_local)\
    X(local_sampled_d32, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, vk_residency_local)

typedef enum
{
    #define X(name, format, samples, usage, residency) texture_memory_kind_##name,
    texture_memory_kind_list
    #undef X
    texture_memory_kind_count
} texture_memory_kind;

static const char *texture_memory_kind_name[] = {
    #define X(name, format, samples, usage, residency) #name,
    texture_memory_kind_list
    #undef X
};

static VkFormat texture_memory_kind_format[] = {
    #define X(name, format, samples, usage, residency) format,
    texture_memory_kind_list
    #undef X
};

static VkSampleCountFlags texture_memory_kind_samples[] = {
    #define X(name, format, samples, usage, residency) samples,
    texture_memory_kind_list
    #undef X
};

static VkImageUsageFlags texture_memory_kind_usage[] = {
    #define X(name, format, samples, usage, residency) usage,
    texture_memory_kind_list
    #undef X
};

static VkMemoryPropertyFlags texture_memory_kind_residency[] = {
    #define X(name, format, samples, usage, residency) residency,
    texture_memory_kind_list
    #undef X
};

typedef struct
{
    texture_memory_kind kind;
    u64 capacity;
} texture_memory_entry;

static buffer_memory_entry sc_buffer_memory_entries[] = {
    { .kind = buffer_memory_kind_coherent_ssbo, .capacity = Mb(32) },
    { .kind = buffer_memory_kind_local_ssbo_dst, .capacity = Mb(32) },
    { .kind = buffer_memory_kind_local_indirect_ssbo_dst, .capacity = Mb(2) },
    { .kind = buffer_memory_kind_coherent_resource_descriptor_buffer, .capacity = Mb(1)},
};

static buffer_memory_entry temp_buffer_memory_entries[] = {
    { .kind = buffer_memory_kind_coherent_src, .capacity = Mb(32) }
};

static buffer_memory_entry fd_buffer_memory_entreis[] = {
    { .kind = buffer_memory_kind_coherent_src, .capacity = Mb(32) }
};

static buffer_memory_entry swpc_buffer_memory_entries[] = {
};

static texture_memory_entry swpc_texture_memory_entries[] = {
    { .kind = texture_memory_kind_local_sampled_r8g8b8a8, .capacity = Mb(128)},
    { .kind = texture_memory_kind_local_sampled_d32, .capacity = Mb(128)}
};

typedef enum
{
    render_geometry_type_static_opaque,
    render_geometry_type_dynamic_opaque,
    render_geometry_type_count
} render_geometry_type;

typedef enum
{
    render_color_target_albedo,
    render_color_target_normal,
    render_color_target_light,
    render_color_target_crt,
    render_color_target_count,
} render_color_target;

typedef struct
{
    deferred_light_pass deferred_light_pass;
    crt_pass crt_pass;

    gpu_texture *depth[frames_in_flight];    
    gpu_texture *color_target[render_color_target_count][frames_in_flight];

    render_command commands[render_geometry_type_count][frames_in_flight];

    camera cam;
} render_full_viewport;

typedef struct
{ 
    gpu_texture *depth[frames_in_flight];
    render_command commands[render_geometry_type_count][frames_in_flight];

    camera cam;
} render_depth_only_viewport;

internal struct
{    
    jgfx_cmd_line_args args;

	arena *static_memory;
	arena *temp_memory;
    arena *scene_memory;

    thread_pool *main_thread_pool;	    
	b8 exit;
	glfw_context glfw_ctx;	
    window *win;
	tick_info tick_info;
    profiler prf;
	gpu_context gpu;
    gpu_profiler gpu_prf;
    basic_ent_manager ent_manager;

    vec2_f64 last_mouse_pos;
    b8 mouse_captured;

    render_manager main_render_manager;
    render_batch static_opaque_batches[frames_in_flight];  
    u32 static_mesh_frames_to_collect;
    
    render_batch dynamic_opaque_batches[frames_in_flight];   

    struct
    {
        present_pass present_pass;
        depth_pass depth_pass;
        gbuffer_pass gbuffer_pass;
        render_command_fill_pass fill_pass;
    } shared_gpu_passes;

    u32 full_viewport_count;
    render_full_viewport *full_viewports;
    u32 main_full_viewport_id;
    render_color_target present_target;

    struct
    {
        gpu_buffer_arena *fd_buffer[frames_in_flight][buffer_memory_kind_count];
        gpu_buffer_arena *temp_buffer[buffer_memory_kind_count];
        gpu_buffer_arena *sc_buffer[buffer_memory_kind_count];
        gpu_buffer_arena *swpc_buffer[buffer_memory_kind_count];

        gpu_texture_arena *swpc_texture[texture_memory_kind_count];
    } gpu_memory;
} env;

#define cpu_n_gpu_profile_scope(cmd, ...)\
    for(struct { profile_scope *cpu_scope; u32 gpu_scope_id; } _prf_scopes = { profiler_scope_begin(&env.prf, text64_make(__VA_ARGS__)), gpu_profiler_scope_begin(&env.gpu_prf, &env.gpu, cmd, text64_make(__VA_ARGS__)) + 1 }; \
        _prf_scopes.gpu_scope_id; \
        _prf_scopes.gpu_scope_id = (gpu_profiler_scope_end(&env.gpu_prf, &env.gpu, cmd, _prf_scopes.gpu_scope_id - 1), profiler_scope_end(_prf_scopes.cpu_scope), 0))


void key_cb(window *win, void* arg, int key, int scancode, int action, int mods)
{
    switch (action) 
    {
        case GLFW_PRESS:
        {
            if(key == GLFW_KEY_F11)
            {
                window_toggle_fullscreen(win);
            }
            else if(key == GLFW_KEY_Z)
            {
                env.present_target = (env.present_target + 1) % render_color_target_count;
            }
            else if(key == GLFW_KEY_Q)
            {
                env.exit = true;
            }
            else if(key == GLFW_KEY_J)
            {
                gpu_context_set_present_mode(&env.gpu, VK_PRESENT_MODE_FIFO_KHR);
            }
            else if(key == GLFW_KEY_H)
            {
                gpu_context_set_present_mode(&env.gpu, VK_PRESENT_MODE_MAILBOX_KHR);
            }
            else if(key == GLFW_KEY_K)
            {
                gpu_context_set_present_mode(&env.gpu, VK_PRESENT_MODE_IMMEDIATE_KHR);
            }
            else if(key == GLFW_KEY_C)
            {
                env.mouse_captured = !env.mouse_captured;
                if(env.mouse_captured)
                {
                    glfwSetInputMode(win->handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);                
                }
                else
                {
                    glfwSetInputMode(win->handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }                        
            break;
        }
    }
}

void spawn_model_node(basic_ent_manager *manager, basic_ent_id parent, render_primitive_node *node, bool dynamic)
{
    basic_ent_id node_ent = ent_spawn(manager);
    ent_add_relation(manager, parent, node_ent);
    ent_add_transform(manager, node_ent);
    *ent_get(manager, node_ent, local_transform) = local_transform_from_mat(&node->local_transform);

    for each_index(primitive_id, node->primitive_count)
    {
        render_primitive *primitive = node->primitives + primitive_id;

        basic_ent_id prim_ent = ent_spawn(manager);        
        ent_add_relation(manager, node_ent, prim_ent);
        ent_add_transform(manager, prim_ent);

        if(dynamic) ent_add(manager, prim_ent, dynamic_mesh, { primitive->mesh_id });
        else        ent_add(manager, prim_ent, static_mesh,  { primitive->mesh_id });
    }
        
    for each_index(node_id, node->node_count)
    {
        render_primitive_node *child_node = node->nodes + node_id;
        spawn_model_node(manager, node_ent, child_node, dynamic);
    }
}

void spawn_model(basic_ent_manager *manager, render_manager *rmanager, render_model_id model_id, basic_ent_id target, b8 dynamic)
{
    for each_index(node_id, rmanager->models[model_id].node_count)
    {
        render_primitive_node *node = rmanager->models[model_id].nodes + node_id;
        spawn_model_node(manager, target, node, dynamic);
    }
}

void refresh_swapchain_targets()
{
    for each_index(i, frames_in_flight)
    {   
        for each_index(vp_id, env.full_viewport_count)
        {
            env.full_viewports[vp_id].depth[i] = gpu_texture_arena_push(
                .arena = env.gpu_memory.swpc_texture[texture_memory_kind_local_sampled_d32], 			
                .tag = text128_make("viewports[%zu].depth[%zu]", vp_id, i),			
                .device = &env.gpu.device, 
                .extent = { env.gpu.swapchain.extent.width, env.gpu.swapchain.extent.height, 1 }, 			
            );	

            for each_index(target_id, render_color_target_count)
            {
                env.full_viewports[vp_id].color_target[target_id][i] = gpu_texture_arena_push(
                    .arena = env.gpu_memory.swpc_texture[texture_memory_kind_local_sampled_r8g8b8a8], 					
                    .tag = text128_make("viewports[%zu].[%u][%zu]", vp_id, target_id, i),			
                    .device = &env.gpu.device, 
                    .extent = { env.gpu.swapchain.extent.width, env.gpu.swapchain.extent.height, 1 }, 			
                );	
            }
        }
    }
}

void env_init_gpu_memory()
{


    for each_index(frame_id, frames_in_flight)
    {
        for each_index(i, arrlen(fd_buffer_memory_entreis))
        {
            buffer_memory_entry entry = fd_buffer_memory_entreis[i];

            env.gpu_memory.fd_buffer[frame_id][entry.kind] = gpu_buffer_arena_make(
                .mem = env.static_memory,
                .name = text128_make("fd_buffer[%u].%s", frame_id, buffer_memory_kind_name[entry.kind]),
                .device = &env.gpu.device,
                .capacity = entry.capacity,
                .usage = buffer_memory_kind_usage[entry.kind],
                .properties = buffer_memory_kind_residency[entry.kind]
            );
        } 	
    }

    for each_index(i, arrlen(temp_buffer_memory_entries))
    {        
        buffer_memory_entry entry = temp_buffer_memory_entries[i];

        env.gpu_memory.temp_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("temp_buffer.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrlen(sc_buffer_memory_entries))
    {        
        buffer_memory_entry entry = sc_buffer_memory_entries[i];

        env.gpu_memory.sc_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("sc_buffer.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrlen(swpc_buffer_memory_entries))
    {        
        buffer_memory_entry entry = swpc_buffer_memory_entries[i];

        env.gpu_memory.swpc_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("swpc_buffer.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrlen(swpc_texture_memory_entries))
    {
        texture_memory_entry entry = swpc_texture_memory_entries[i];

        env.gpu_memory.swpc_texture[entry.kind] = gpu_texture_arena_make(
            .mem = env.static_memory,	
			.name = text128_make("swpc_texture.%s", texture_memory_kind_name[entry.kind]),
			.device = &env.gpu.device,			 			
			.capacity = entry.capacity, 
			.format = texture_memory_kind_format[entry.kind],
			.samples = texture_memory_kind_samples[entry.kind],
			.flags = 0,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = texture_memory_kind_usage[entry.kind],
			.properties = texture_memory_kind_residency[entry.kind],
			.sharing_mode = VK_SHARING_MODE_EXCLUSIVE
		); 	
    }
}

void env_init_full_viewports(u32 count)
{
    env.full_viewport_count = count;

    env.full_viewports = arena_ppush_arr(env.static_memory, render_full_viewport, env.full_viewport_count);
    env.main_full_viewport_id = 0;

    env.full_viewports[env.main_full_viewport_id].cam.near = 0.1f;
    env.full_viewports[env.main_full_viewport_id].cam.far = 1000.0f;
    env.full_viewports[env.main_full_viewport_id].cam.fov = 90.0f;
    env.full_viewports[env.main_full_viewport_id].cam.ratio = 1.0f;

    for each_index(i, frames_in_flight)
    {
        env.static_opaque_batches[i] = render_batch_make(
            .tag = text128_make("env.static_opaque_batches[%u]", i), 
            .manager = &env.main_render_manager, 
            .gpu = &env.gpu,  
            .any_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_local_ssbo_dst],
            .mem = env.static_memory,
            .instance_count = 103 * env.args.obj_grid_size * env.args.obj_grid_size
        );

        env.dynamic_opaque_batches[i] = render_batch_make(
            .tag = text128_make("env.dynamic_opaque_batches[%u]", i), 
            .manager = &env.main_render_manager, 
            .gpu = &env.gpu,  
            .any_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_ssbo],        
            .mem = env.static_memory,
            .instance_count = 103 * env.args.obj_grid_size * env.args.obj_grid_size
        );

        for each_index(viewport_id, env.full_viewport_count)
        {            
            env.full_viewports[viewport_id].commands[render_geometry_type_static_opaque][i] = render_command_make(
                .tag = text128_make("env.full_static_opaque_view[%u]", i), 
                .manager = &env.main_render_manager,
                .batch = &env.static_opaque_batches[i], 
                .local_indirect_command_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_local_indirect_ssbo_dst],
                .coherent_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_ssbo],                
                .local_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_local_ssbo_dst],
                .gpu = &env.gpu,  
                .mem = env.static_memory
            );
    
            env.full_viewports[viewport_id].commands[render_geometry_type_dynamic_opaque][i] = render_command_make(
                .manager = &env.main_render_manager,
                .batch = &env.dynamic_opaque_batches[i], 
                .tag =  text128_make("env.full_dynamic_opaque_view[%u]", i), 
                .local_indirect_command_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_local_indirect_ssbo_dst],
                .coherent_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_ssbo],                
                .local_ssbo_arena = env.gpu_memory.sc_buffer[buffer_memory_kind_local_ssbo_dst],
                .gpu = &env.gpu,  
                .mem = env.static_memory
            );
        }
    }
}

void spawn_scene(basic_ent_manager *manager, render_manager *rmanager, render_model_id model)
{   
    basic_ent_id p = ent_spawn(manager);
    ent_name_set(manager, p, "player : %u", p);
    ent_add_transform(manager, p);
    ent_add(manager, p, camera_controller, {});            
    
    for each_index(table_id, manager->table_count)
    {
        if(ent_has_from_id(manager, p, table_id))
        {
            msg("%s has %s", ent_name(manager, p).data, ent_table_name_from_id(manager, table_id));
        }
    }

    for each_index(x, env.args.obj_grid_size)
    {
        for each_index(y, env.args.obj_grid_size)
        {
            basic_ent_id obj = ent_spawn(manager);

            b8 is_dynamic = obj % 2;
            if(is_dynamic)
            {
                ent_add(manager, obj, dude, {});                             
                ent_name_set(manager, obj, "dynamic_obj : %u", obj);
            }              
            else
            {
                ent_name_set(manager, obj, "static_obj : %u", obj);
            }    
            ent_add_transform(manager, obj,
                .pos = {-250 + (x / (f32)env.args.obj_grid_size) * 500, 0, -250 + (y / (f32)env.args.obj_grid_size) * 500},
                .rot = {0,0,0}
            );                
            
            spawn_model(manager, rmanager, model, obj, is_dynamic);
        }
    }

    msg("ent count = %u", manager->active_count);
    msg("ent count (static_mesh) = %u", ent_table_entries(manager, static_mesh));
    msg("ent count (dynamic_mesh) = %u", ent_table_entries(manager, dynamic_mesh));
    msg("ent count (ent_root) = %u", ent_table_entries(manager, ent_root));
}   

void env_init(jgfx_cmd_line_args args)
{   
    env.args = args;  

    env.static_memory = arena_make(
        .capacity = Gb(64),
        .name = text64_make("static_memory(env)")
    );

    env.temp_memory = arena_make(
        .capacity = Gb(64),
        .name = text64_make("temp_memory(env)")
    );

    env.scene_memory = arena_make(
        .capacity = Gb(64),
        .name = text64_make("scene_memory(env)")
    );

    env.main_thread_pool = thread_pool_make(
        .mem = env.static_memory, 
        .name = text64_make("env.main_thread_pool"), 
        .size = os_fetch_hardware_info().logical_processors
    );

    env.ent_manager = ent_manager_make(  
        basic,      
        .mem = env.static_memory,
        .name = text128_make("env.ent_manager"),
        .capacity = 103 * env.args.obj_grid_size * env.args.obj_grid_size + 64,
        .names = true,
        .relations = true,
    );    

    env.glfw_ctx = glfw_context_make();
    env.tick_info = (tick_info){0};

    env.win = window_make(
        .mem = env.static_memory,
        .size = { 800, 800 },
        .pos = { 200, 200 },		        
        .title = text64_make("jgfx - (%s, %s)", jahoda_platform_string, is_debug ? "debug" : "release"),
        .invisible = true,
        .fullscreen = false,
        .callback = {
            .key_fn = key_cb             
        }
    );

    env.gpu = gpu_context_make(
        .static_arena = env.static_memory,
        .temp_arena = env.temp_memory,
        .win = env.win,
        .present_mode = VK_PRESENT_MODE_FIFO_KHR
    );
        
    env.gpu_prf = gpu_profiler_make(&env.gpu);

    env_init_gpu_memory();

    env.main_render_manager = render_manager_make(
        .name = text64_make("main"),
        .gpu = &env.gpu,
        .mem = env.static_memory,
        .model_capacity = 1,
        .mesh_capacity = 128,
        .texture_capacity = 128,
        .index_memory = Mb(16),
        .vertex_memory = Mb(16),
        .rgb8_memory = Mb(16)
    );

    render_model_id sponza_model = render_manager_load_gltf(
        &env.main_render_manager, 
        &env.gpu,
        env.gpu_memory.temp_buffer[buffer_memory_kind_coherent_src],
        env.static_memory, 
        env.temp_memory, 
        str8_lit("assets/models/Sponza/Sponza.gltf"),
        render_scene_default
    );

    env_init_full_viewports(1);

    refresh_swapchain_targets();

    env.static_mesh_frames_to_collect = frames_in_flight;

    spawn_scene(&env.ent_manager, &env.main_render_manager, sponza_model);
}

void env_release()
{
    gpu_device_wait_idle(&env.gpu.device);
    // present_pass_release(&env.present_pass, &env.gpu);
    // deferred_light_pass_release(&env.deferred_light_pass, &env.gpu);
    // gbuffer_pass_release(&env.gbuffer_pass, &env.gpu);
    // depth_pass_release(&env.depth_pass, &env.gpu);
    // render_manager_release(&env.main_render_manager, &env.gpu);
    gpu_profiler_release(&env.gpu_prf, &env.gpu);
    gpu_context_release(&env.gpu);
    window_release(env.win);
    glfw_context_release(&env.glfw_ctx);
    arena_release(env.temp_memory);
    arena_release(env.static_memory);
}

void collect_dynamic_mesh(
    gpu_context *gpu,  
    render_batch batches per_frame_in_flight,
    basic_ent_manager *entities)
{
    render_batch *batch = render_batch_reset(batches + gpu->frame_index);
    
    basic_ent_query q = ent_query_make(            
        entities,
        .include_mask = ent_bitmask_make(entities, dynamic_mesh)
    );

    while(ent_query_next(&q))
    {                        
        u32 local_geometry_id = ent_local_id(entities, q.id, dynamic_mesh);
        render_mesh_id mesh_id = ent_get(entities, q.id, dynamic_mesh)->mesh_id;

        ((render_transform*)batch->transform_buffer->mem)[local_geometry_id].mat = ent_get(entities, q.id, global_transform)->mat;
        ((render_mesh_id*)batch->mesh_id_buffer->mem)[local_geometry_id] = mesh_id;
        batch->instance_counts[mesh_id]++;
    }
}

void collect_static_mesh_record(
    VkCommandBuffer cmd, 
    gpu_context *gpu,  
    render_batch batches per_frame_in_flight,
    basic_ent_manager *entities)
{
    render_batch *batch = render_batch_reset(batches + gpu->frame_index);    

    gpu_buffer *instance_transform_stagin = gpu_buffer_arena_push(
        render_transform,
        .tag = text128_make("[%s] instance_transform_stagin", __func__),
        .arena = env.gpu_memory.fd_buffer[gpu->frame_index][buffer_memory_kind_coherent_src],
        .device = &gpu->device,
        .size = batch->instance_capacity
    );

    gpu_buffer *instance_mesh_stagin = gpu_buffer_arena_push(
        render_mesh_id,
        .tag = text128_make("[%s] instance_mesh_stagin", __func__),
        .arena = env.gpu_memory.fd_buffer[gpu->frame_index][buffer_memory_kind_coherent_src],
        .device = &gpu->device,
        .size = batch->instance_capacity
    );

    basic_ent_query q = ent_query_make(            
        entities,
        .include_mask = ent_bitmask_make(entities, static_mesh)
    );

    while(ent_query_next(&q))
    {                        
        u32 local_geometry_id = ent_local_id(entities, q.id, static_mesh);
        render_mesh_id mesh_id = ent_get(entities, q.id, static_mesh)->mesh_id;

        ((render_transform*)instance_transform_stagin->mem)[local_geometry_id].mat = ent_get(entities, q.id, global_transform)->mat;
        ((render_mesh_id*)instance_mesh_stagin->mem)[local_geometry_id] = mesh_id;
        batch->instance_counts[mesh_id]++;
    }

    vku_cmd_copy_buffer(cmd, instance_transform_stagin, batch->transform_buffer, 0, 0);
    vku_cmd_copy_buffer(cmd, instance_mesh_stagin, batch->mesh_id_buffer, 0, 0);

    vku_cmd_memory_barrier(
        cmd,

        VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT, 
        
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT
    );    
}

void transform_prop_handler_helper(basic_ent_manager * restrict manager, mat4 * restrict parent_mat, basic_ent_id id)
{               
    mat4_f32_mul_fast(
        parent_mat, 
        local_transform_as_mat(ent_get(manager, id, local_transform)), 
        &ent_get(manager, id, global_transform)->mat
    );
    
    for ent_each_child(manager, ch, id)
    {            
        transform_prop_handler_helper(manager, &ent_get(manager, id, global_transform)->mat, *ch);
    }  
}

thread_pool_task_declare(
    transform_prop,
    { basic_ent_manager *manager; basic_ent_query *query; },
    {        
        while(ent_query_next(args->query))
        {                    
            global_transform * restrict gtr = ent_get(args->manager, args->query->id, global_transform);
            local_transform * restrict ltr = ent_get(args->manager, args->query->id, local_transform);
            
            gtr->mat = *local_transform_as_mat(ltr);
            
            for ent_each_child(args->manager, ch, args->query->id)
            {
                transform_prop_handler_helper(args->manager, &gtr->mat, *ch);                    
            }                
        }

        return NULL;
    }       
)

void transform_prop_handler(
    basic_ent_manager *manager, 
    arena *temp_mem, 
    thread_pool *pool)
{
    basic_ent_query full_query = ent_query_make(   
        manager,   
        .include_mask = ent_bitmask_make(
            manager,
            local_transform, 
            global_transform,
            ent_root
        ),
    );        

    basic_ent_query *queries = ent_query_split(
        &full_query,
        pool->size,
        temp_mem     
    );        

    for each_index(i, pool->size)
    {
        transform_prop_args *arg = arena_ppush(temp_mem, transform_prop_args);

        arg->manager = manager;
        arg->query = queries + i;

        thread_pool_schedule(
            pool, 
            transform_prop_task_make(arg)
        );
    }

    thread_pool_wait_all(pool);
}

void main_camera_update()
{
    if(env.gpu.swapchain_was_refreshed)
    {
        env.full_viewports[env.main_full_viewport_id].cam.ratio = (f32)env.gpu.swapchain.extent.width / (f32)env.gpu.swapchain.extent.height;        
    }

    vec2_f64 current_mouse_pos = window_mouse_pos(env.win);

    vec2_f64 mouse_delta = vec2_f64_sub(current_mouse_pos, env.last_mouse_pos);

    env.last_mouse_pos.x = current_mouse_pos.x;
    env.last_mouse_pos.y = current_mouse_pos.y;

    const f32 mouse_sensitivity = 0.002f; 

    basic_ent_query q = ent_query_make(
        &env.ent_manager,
        .include_mask = ent_bitmask_make(&env.ent_manager, camera_controller)
    );

    ent_query_next(&q);
    basic_ent_id controller_id = q.id;
    dbg_verify(ent_query_next(&q));

    if (env.mouse_captured)
    {                        
        local_transform *ltr = ent_get(&env.ent_manager, q.id, local_transform);
        
        ltr->rot.y -= (f32)mouse_delta.x * mouse_sensitivity; // yaw
        ltr->rot.x -= (f32)mouse_delta.y * mouse_sensitivity; // pitch

        f32 max_pitch = 1.55f; 
        if (ltr->rot.x > max_pitch) ltr->rot.x = max_pitch;
        if (ltr->rot.x < -max_pitch) ltr->rot.x = -max_pitch;
        
        f32 velocity = (window_key_down(env.win, GLFW_KEY_LEFT_SHIFT) ? 30.0f : 15.0f) * env.tick_info.dt; 

        f32 yaw = ltr->rot.y;
        
        f32 fwd_x = -sinf(yaw);
        f32 fwd_z = -cosf(yaw);
        
        f32 right_x = cosf(yaw);
        f32 right_z = -sinf(yaw);

        if (window_key_down(env.win, GLFW_KEY_W)) 
        {
            ltr->pos.x += fwd_x * velocity;
            ltr->pos.z += fwd_z * velocity;
        }
        if (window_key_down(env.win, GLFW_KEY_S)) 
        {
            ltr->pos.x -= fwd_x * velocity;
            ltr->pos.z -= fwd_z * velocity;
        }

        if (window_key_down(env.win, GLFW_KEY_A)) 
        {
            ltr->pos.x -= right_x * velocity;
            ltr->pos.z -= right_z * velocity;
        }
        if (window_key_down(env.win, GLFW_KEY_D)) 
        {
            ltr->pos.x += right_x * velocity;
            ltr->pos.z += right_z * velocity;
        }

        if (window_key_down(env.win, GLFW_KEY_SPACE)) 
        {
            ltr->pos.y += velocity; 
        }
        if (window_key_down(env.win, GLFW_KEY_LEFT_CONTROL)) 
        {
            ltr->pos.y -= velocity; 
        }

        camara_update(
            &env.full_viewports[env.main_full_viewport_id].cam, 
            ent_get(&env.ent_manager, q.id, global_transform)->mat
        ); 
    }
}

void events()
{
    window_poll_events(env.win);   

    if (window_is_minimized(env.win)) 
    {
        window_wait_events(env.win);
    }
}
    
void move_stuff()
{
    basic_ent_query q = ent_query_make(
        &env.ent_manager,
        .include_mask = ent_bitmask_make(&env.ent_manager, dude, ent_root)
    );

    while(ent_query_next(&q))
    {            
        local_transform *ltr = ent_get(&env.ent_manager, q.id, local_transform);

        f32 x = ltr->pos.x;
        f32 z = ltr->pos.z;
        
        f32 phase = (f32)fmod(env.tick_info.time * 2.0 + x * 0.05 + z * 0.05, 2.0 * jahoda_math_pi);
        f32 y = sinf(phase) * 5.0f;

        local_transform_set_pos(ltr, (vec3){ .x = x, .y = y, .z = z });
    }
}

void update()
{    
    env.exit = env.exit || window_should_close(env.win);

    tick_info_update(&env.tick_info);  

    profile_scope(&env.prf, "move_stuff")
    {
        move_stuff();   
    }

    profile_scope(&env.prf,"global_transform_handler")
    {
        transform_prop_handler(&env.ent_manager, env.temp_memory, env.main_thread_pool);
    }

    main_camera_update();
}

void render_depth_only_viewport_record(VkCommandBuffer cmd, render_depth_only_viewport *vp)
{
    cpu_n_gpu_profile_scope(cmd, "render_batch_fill_pass_record (depth_only vp = %x)", vp)
    {
        render_command_fill_pass_record(
            cmd,
            &env.shared_gpu_passes.fill_pass,
            env.temp_memory,
            &env.gpu,
            &env.main_render_manager,  
            render_geometry_type_count,            
            vp->commands,
            &vp->cam
        );
    }

    cpu_n_gpu_profile_scope(cmd, "depth_pass_record (depth_only vp = %x)", vp)    
    {
        depth_pass_record(
            cmd,
            &env.shared_gpu_passes.depth_pass,
            &env.gpu, 
            env.temp_memory,
            &env.main_render_manager.geometry,
            vp->depth,
            render_geometry_type_count,
            vp->commands,
            &vp->cam
        );
    }
}

void render_full_viewport_record(VkCommandBuffer cmd, render_full_viewport *vp)
{
    cpu_n_gpu_profile_scope(cmd, "render_batch_fill_pass_record (full vp = %x)", vp)
    {
        render_command_fill_pass_record(
            cmd,
            &env.shared_gpu_passes.fill_pass,
            env.temp_memory,
            &env.gpu,
            &env.main_render_manager,  
            render_geometry_type_count,
            vp->commands,
            &vp->cam
        );    
    }

    cpu_n_gpu_profile_scope(cmd, "depth_pass_record (full vp = %x)", vp)    
    {
        depth_pass_record(
            cmd,
            &env.shared_gpu_passes.depth_pass,
            &env.gpu, 
            env.temp_memory,
            &env.main_render_manager.geometry,
            vp->depth,
            render_geometry_type_count,
            vp->commands,
            &vp->cam
        );
    }
    
    cpu_n_gpu_profile_scope(cmd, "gbuffer_pass_record (full vp = %x)", vp)
    {
        gbuffer_pass_record(
            cmd,
            &env.shared_gpu_passes.gbuffer_pass, 
            &env.gpu,
            env.temp_memory,
            &env.main_render_manager.geometry,
            vp->depth,
            vp->color_target[render_color_target_albedo],
            vp->color_target[render_color_target_normal],
            render_geometry_type_count,
            vp->commands,
            &vp->cam
            );
    }


    cpu_n_gpu_profile_scope(cmd, "deferred_light_pass_record (full vp = %x)", vp)
    {
        deferred_light_pass_record(
            cmd,
            &vp->deferred_light_pass, 
            &env.gpu, 
            env.temp_memory, 
            env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            vp->depth,
            vp->color_target[render_color_target_albedo],
            vp->color_target[render_color_target_normal],
            vp->color_target[render_color_target_light],
            &vp->cam
        );
    }

    cpu_n_gpu_profile_scope(cmd, "crt_pass_record (full vp = %x)", vp)
    {
        crt_params params = crt_params_default;

        params.resolution = (vec2_f32){
            vp->color_target[render_color_target_light][env.gpu.frame_index]->extent.width, 
            vp->color_target[render_color_target_light][env.gpu.frame_index]->extent.height
        };

        params.time = env.tick_info.time;
        params.aberration = ((sin(env.tick_info.time) + 1.0f) / 2.0f) * 0.050f + 0.025f;
        params.mask_strength = params.aberration * 10.0f;
        params.curvature = params.aberration * 10;

        crt_pass_record(
            cmd,
            &vp->crt_pass,             
            &env.gpu, 
            env.temp_memory,             
            env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            env.tick_info.time,
            vp->color_target[render_color_target_light],
            vp->color_target[render_color_target_crt],
            params
        );
    }
}

void present_record(VkCommandBuffer cmd, gpu_texture *texture[frames_in_flight])
{
    cpu_n_gpu_profile_scope(cmd, "present_pass_record")
    {
        present_pass_record(
            cmd,
            &env.shared_gpu_passes.present_pass, 
            &env.gpu, 
            env.temp_memory,
            env.gpu_memory.sc_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            texture
        );
    }    
}

void collect_geometry_record(VkCommandBuffer cmd)
{
    if(env.static_mesh_frames_to_collect)
    {
        cpu_n_gpu_profile_scope(cmd, "collect_static_mesh_record")
        {
            collect_static_mesh_record(
                cmd,
                &env.gpu, 
                env.static_opaque_batches,
                &env.ent_manager
            );
        }

        env.static_mesh_frames_to_collect--;
    }

    cpu_n_gpu_profile_scope(cmd, "collect_dynamic_mesh")
    {
        collect_dynamic_mesh(
            &env.gpu, 
            env.dynamic_opaque_batches,
            &env.ent_manager
        );
    }
}

void render()
{   
    if (window_is_minimized(env.win)) { return; } 

    gpu_context_drain(&env.gpu);

    for each_index(i, arrlen(fd_buffer_memory_entreis))
    {    
        buffer_memory_entry entry = fd_buffer_memory_entreis[i];
        gpu_buffer_arena_reset(env.gpu_memory.fd_buffer[env.gpu.frame_index][entry.kind], &env.gpu.device);
    }

    for each_index(i, arrlen(temp_buffer_memory_entries))
    {
        buffer_memory_entry entry = temp_buffer_memory_entries[i];
        gpu_buffer_arena_reset(env.gpu_memory.temp_buffer[entry.kind], &env.gpu.device);
    }

    if(gpu_context_try_begin_frame(&env.gpu, env.win, env.temp_memory))
    {
        if(env.gpu.swapchain_was_refreshed)
        {
            for each_index(i, arrlen(swpc_buffer_memory_entries))
            {
                buffer_memory_entry entry = swpc_buffer_memory_entries[i];
                gpu_buffer_arena_reset(env.gpu_memory.swpc_buffer[entry.kind], &env.gpu.device);
            }
            
            for each_index(i, arrlen(swpc_texture_memory_entries))
            {
                texture_memory_entry entry = swpc_texture_memory_entries[i];
                gpu_texture_arena_reset(env.gpu_memory.swpc_texture[entry.kind], &env.gpu.device);
            }
            
            refresh_swapchain_targets();
        }

        gpu_profiler_resolve(&env.gpu_prf, &env.gpu);

        VkCommandBuffer cmd = gpu_context_grab_graphics(&env.gpu);

        gpu_profiler_reset(&env.gpu_prf, &env.gpu, cmd);
        
        collect_geometry_record(cmd);
        
        render_full_viewport_record(
            cmd, 
            &env.full_viewports[env.main_full_viewport_id]
        );

        present_record(
            cmd,
            env.full_viewports[env.main_full_viewport_id].color_target[env.present_target]
        );

        vku_cmd_textures_transition_state(  
            cmd,
            env.temp_memory, 
            { env.gpu.swapchain.textures + env.gpu.swapchain_image_index, gpu_texture_state_present }
        );
        
        gpu_context_commit_graphics(&env.gpu, cmd);
        
        profile_scope(&env.prf,"gpu_context_end_and_present_frame")
        { 
            gpu_context_end_and_present_frame(&env.gpu, env.temp_memory);		
        }        
    }    
}

int jgfx_run(int argc, char **argv)
{   
    jgfx_cmd_line_args args = jgfx_cmd_line_args_parse(argc, argv);

    if(args.h)
    {
        jgfx_cmd_line_args_display();
        return 0;
    }

    if(args.log_provided)
    {
        log_dest(
            (strcmp(args.log, "stdout") == 0) ? stdout : fopen(args.log, "w")
        );
    }

    env_init(args);
    
	window_make_visible(env.win);
   
    while (!env.exit)
    {     
        profiler_reset(&env.prf);

        profile_scope(&env.prf,"frame %u", env.tick_info.tick)
        {            
            events();
            update();
            render();

            msg("env.temp_memory current = %.2lf Kb", as_Kb(env.temp_memory->current));
            
            arena_reset(env.temp_memory); 
        }

        for each_index(i, env.gpu_prf.scopes_occupied[env.gpu.frame_index])
        {
            msgl(gpu_prf, gpu_profile_scope_fmt, gpu_profile_scope_arg(env.gpu_prf.scopes[env.gpu.frame_index] + i));
        }

        for each_index(i, env.prf.scopes_occupied)
        {
            msgl(cpu_prf, profile_scope_fmt, profile_scope_arg(env.prf.scopes + i));
        }
    }
    
    profile_scope(&env.prf,"env_release")
    {
        env_release();    
    }
}