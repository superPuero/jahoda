#include <jahoda/core/core.h>
#include <jahoda/core/thread_pool.h>
#include <jahoda/gfx/window.h>
#include <jahoda/gfx/gpu_context.h>
#include <jahoda/gfx/present_pass.h>
#include <jahoda/gfx/render_manager.h>
#include <jahoda/base/tick_info.h>
#include <jahoda/core/os.h>
#include <jahoda/core/tmap.h>

#include "depth_pass.h"
#include "gbuffer_pass.h"
#include "deferred_light_pass.h"
#include "ent.h"
#include "basic_pass.h"

typedef struct
{
    vec2_f32 resolution;
	f32 time;
    f32 curvature;       
    f32 scanline_weight; 
    f32 mask_strength;   
    f32 aberration;      
    f32 bloom_boost
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

static buffer_memory_entry scene_bound_buffer_memory_entries[] = {
    { .kind = buffer_memory_kind_coherent_ssbo, .capacity = Mb(32) },
    { .kind = buffer_memory_kind_local_ssbo_dst, .capacity = Mb(32) },
    { .kind = buffer_memory_kind_local_indirect_ssbo_dst, .capacity = Mb(2) },
    { .kind = buffer_memory_kind_coherent_resource_descriptor_buffer, .capacity = Mb(1)},
};

static buffer_memory_entry temp_buffer_memory_entries[] = {
    { .kind = buffer_memory_kind_coherent_src, .capacity = Mb(32) }
};

static buffer_memory_entry frame_deferred_buffer_memory_entreis[] = {
    { .kind = buffer_memory_kind_coherent_src, .capacity = Mb(32) }
};

static buffer_memory_entry swapchain_bound_buffer_memory_entries[] = {
};

static texture_memory_entry swapchain_bound_texture_memory_entries[] = {
    { .kind = texture_memory_kind_local_sampled_r8g8b8a8, .capacity = Mb(128)},
    { .kind = texture_memory_kind_local_sampled_d32, .capacity = Mb(128)}
};

typedef enum
{
    render_geometry_type_static_opaque,
    render_geometry_type_volatile_opaque,
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
	arena *static_memory;
	arena *temp_memory;
    arena *scene_memory;

    thread_pool *main_thread_pool;	    
	b8 exit;
	glfw_context glfw_ctx;
	window *win;
	tick_info tick_info;
	gpu_context gpu;
        
    ent_manager ent_manager;

    render_manager main_render_manager;
    render_batch static_opaque_batches[frames_in_flight];  
    u32 static_geometry_frames_to_collect;
    
    render_batch volatile_opaque_batches[frames_in_flight];   

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
    
    struct
    {
        gpu_buffer_arena *frame_deferred_buffer[frames_in_flight][buffer_memory_kind_count];
        gpu_buffer_arena *temp_buffer[buffer_memory_kind_count];
        gpu_buffer_arena *scene_bound_buffer[buffer_memory_kind_count];
        gpu_buffer_arena *swapchain_bound_buffer[buffer_memory_kind_count];

        gpu_texture_arena *swapchain_bound_texture[texture_memory_kind_count];
    } gpu_memory;

    f64 last_mouse_x;
    f64 last_mouse_y;
    b8 mouse_captured;
    b8 not_first_mouse;
} env;

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
                    env.not_first_mouse = false;
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

void ent_put_node( ent_manager *emanager, ent_id ent, render_manager *rmanager, render_primitive_node *node, bool static_)
{
    ent_session(emanager)
    {
        ent_id e = ent_spawn_with(
            ent_with_parent(ent);
            
            ent_with_transform_v(
                local_transform_from_mat(&node->local_transform)
            );

            for each_index(primitive_id, node->primitive_count)
            {
                render_primitive *primitive = node->primitives + primitive_id;
                
                ent_with_children(
                    ent_spawn_with(
                        ent_with_v(render_mesh_id, primitive->mesh_id);

                        if(static_)
                        {
                            ent_with(static_geometry);
                        }
                        else
                        {
                            ent_with(volatile_geometry);
                        }

                        ent_with_transform();                    
                    )
                );                    
            }
        );

        
        for each_index(node_id, node->node_count)
        {
            render_primitive_node *node_ = node->nodes + node_id;
            ent_put_node(emanager, e, rmanager, node_, static_);
        }
    }
}


#define ent_with_model(render_manager, model, static) ({\
    for each_index(node_id, (render_manager)->models[model].node_count)\
    {\
        render_primitive_node *node = (render_manager)->models[model].nodes + node_id;\
        ent_put_node(_session_ent_manager, _session_ent, render_manager, node, static);\
    }\
})

void refresh_swapchain_targets()
{
    for each_index(i, frames_in_flight)
    {   
        for each_index(vp_id, env.full_viewport_count)
        {
            env.full_viewports[vp_id].depth[i] = gpu_texture_arena_push(
                .arena = env.gpu_memory.swapchain_bound_texture[texture_memory_kind_local_sampled_d32], 			
                .tag = text128_make("viewports[%zu].depth[%zu]", vp_id, i),			
                .device = &env.gpu.device, 
                .extent = { env.gpu.swapchain.extent.width, env.gpu.swapchain.extent.height, 1 }, 			
            );	

            for each_index(target_id, render_color_target_count)
            {
                env.full_viewports[vp_id].color_target[target_id][i] = gpu_texture_arena_push(
                    .arena = env.gpu_memory.swapchain_bound_texture[texture_memory_kind_local_sampled_r8g8b8a8], 					
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
        for each_index(i, arrsize(frame_deferred_buffer_memory_entreis))
        {
            buffer_memory_entry entry = frame_deferred_buffer_memory_entreis[i];

            env.gpu_memory.frame_deferred_buffer[frame_id][entry.kind] = gpu_buffer_arena_make(
                .mem = env.static_memory,
                .name = text128_make("frame_deferred_buffer[%u].%s", frame_id, buffer_memory_kind_name[entry.kind]),
                .device = &env.gpu.device,
                .capacity = entry.capacity,
                .usage = buffer_memory_kind_usage[entry.kind],
                .properties = buffer_memory_kind_residency[entry.kind]
            );
        } 	
    }

    for each_index(i, arrsize(temp_buffer_memory_entries))
    {        
        buffer_memory_entry entry = temp_buffer_memory_entries[i];

        env.gpu_memory.temp_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("temp.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrsize(scene_bound_buffer_memory_entries))
    {        
        buffer_memory_entry entry = scene_bound_buffer_memory_entries[i];

        env.gpu_memory.scene_bound_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("scene_bound.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrsize(swapchain_bound_buffer_memory_entries))
    {        
        buffer_memory_entry entry = swapchain_bound_buffer_memory_entries[i];

        env.gpu_memory.swapchain_bound_buffer[entry.kind] = gpu_buffer_arena_make(
            .mem = env.static_memory,
            .name = text128_make("scene_bound.%s", buffer_memory_kind_name[entry.kind]),
            .device = &env.gpu.device,
            .capacity = entry.capacity,
            .usage = buffer_memory_kind_usage[entry.kind],
            .properties = buffer_memory_kind_residency[entry.kind]
        ); 	
    }

    for each_index(i, arrsize(swapchain_bound_texture_memory_entries))
    {
        texture_memory_entry entry = swapchain_bound_texture_memory_entries[i];

        env.gpu_memory.swapchain_bound_texture[entry.kind] = gpu_texture_arena_make(
            .mem = env.static_memory,	
			.name = text128_make("swapchain_bound_texture.%s", texture_memory_kind_name[entry.kind]),
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
            .any_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_local_ssbo_dst],
            .mem = env.static_memory,
            .instance_count = 103 * 15 * 15
        );

        env.volatile_opaque_batches[i] = render_batch_make(
            .tag = text128_make("env.volatile_opaque_batches[%u]", i), 
            .manager = &env.main_render_manager, 
            .gpu = &env.gpu,  
            .any_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_ssbo],
            
            .mem = env.static_memory,
            .instance_count = 103 * 15 * 15
        );
        
        for each_index(viewport_id, env.full_viewport_count)
        {            
            env.full_viewports[viewport_id].commands[render_geometry_type_static_opaque][i] = render_command_make(
                .tag = text128_make("env.static_opaque_view[%u]", i), 
                .manager = &env.main_render_manager,
                .batch = &env.static_opaque_batches[i], 
                .local_indirect_command_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_local_indirect_ssbo_dst],
                .coherent_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_ssbo],                
                .local_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_local_ssbo_dst],
                .gpu = &env.gpu,  
                .mem = env.static_memory
            );
    
            env.full_viewports[viewport_id].commands[render_geometry_type_volatile_opaque][i] = render_command_make(
                .manager = &env.main_render_manager,
                .batch = &env.volatile_opaque_batches[i], 
                .tag =  text128_make("env.volatile_opaque_view[%u]", i), 
                .local_indirect_command_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_local_indirect_ssbo_dst],
                .coherent_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_ssbo],                
                .local_ssbo_arena = env.gpu_memory.scene_bound_buffer[buffer_memory_kind_local_ssbo_dst],
                .gpu = &env.gpu,  
                .mem = env.static_memory
            );
        }
    }
}

void env_init(int argc, char **argv)
{    
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
        .size = os_fetch_hardware_info().logical_processors - 1
    );

    env.ent_manager = ent_manager_make(env.static_memory, 100000);    

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

    msg("%llu", env.main_render_manager.geometry.positions->handle);

    env_init_full_viewports(1);

    refresh_swapchain_targets();

    ent_session(&env.ent_manager)
    {      
        ent_spawn_with(
            ent_with_name("player : %u", ent_with_id());
            ent_with_transform();
            ent_with_children(
                ent_spawn_with(
                    ent_with_name("player : %u", ent_with_id());
                    ent_with_transform();                       
                    ent_with(camera_controller);            
                ),
                ent_spawn_with(
                    ent_with_name("player : %u", ent_with_id());
                    ent_with_transform();                       
                    ent_with(camera_controller);            
                )
            );            
            ent_with(camera_controller);            
        );      

        u32 obj_grid_size = 15;

        for each_index(x, obj_grid_size)
        {
            for each_index(y, obj_grid_size)
            {
                ent_spawn_with(
                    ent_with_name("static_obj : %u", ent_with_id());                
                    ent_with_transform(
                        .pos = {-250 + (x / (f32)obj_grid_size) * 500, 0, -250 + (y / (f32)obj_grid_size) * 500},
                        .rot = {0,0,0}
                    );
                    ent_with_model(&env.main_render_manager, sponza_model, ent_with_id() % 2);
                );      
            }
        }

        msg("ent count = %u", env.ent_manager.active_count);
        msg("ent count (static_geometry) = %u", ent_table_entries(static_geometry));
        msg("ent count (volatile_geometry) = %u", ent_table_entries(volatile_geometry));
    }

    env.static_geometry_frames_to_collect = frames_in_flight;
}

void env_release()
{
    gpu_device_wait_idle(&env.gpu.device);
    // present_pass_release(&env.present_pass, &env.gpu);
    // deferred_light_pass_release(&env.deferred_light_pass, &env.gpu);
    // gbuffer_pass_release(&env.gbuffer_pass, &env.gpu);
    // depth_pass_release(&env.depth_pass, &env.gpu);
    // render_manager_release(&env.main_render_manager, &env.gpu);
    
    gpu_context_release(&env.gpu);
    window_release(env.win);
    glfw_context_release(&env.glfw_ctx);
    arena_release(env.temp_memory);
    arena_release(env.static_memory);
}

void collect_volatile_geometry(
    gpu_context *gpu,  
    render_batch batches per_frame_in_flight,
    ent_manager *entities)
{
    render_batch *batch = render_batch_reset(batches + gpu->frame_index);

    ent_session(entities)
    {        
        for ent_each_with(query, volatile_geometry)
        {                        
            u32 local_geometry_id = ent_local_id_0_based(query.id, volatile_geometry);
            
            ((render_transform*)batch->transform_buffer->mem)[local_geometry_id].mat = query.global_transform->mat;
            ((render_mesh_id*)batch->mesh_id_buffer->mem)[local_geometry_id] = *query.render_mesh_id;
            batch->instance_counts[*query.render_mesh_id]++;
        }
    }    
}

void collect_static_geometry_record(
    VkCommandBuffer cmd, 
    gpu_context *gpu,  
    render_batch batches per_frame_in_flight,
    ent_manager *entities)
{
    render_batch *batch = render_batch_reset(batches + gpu->frame_index);

    gpu_buffer *instance_transform_stagin = gpu_buffer_arena_push(
        render_transform,
        .tag = text128_make("[%s] instance_transform_stagin", __func__),
        .arena = env.gpu_memory.frame_deferred_buffer[gpu->frame_index][buffer_memory_kind_coherent_src],
        .device = &gpu->device,
        .size = batch->instance_capacity
    );

    gpu_buffer *instance_mesh_stagin = gpu_buffer_arena_push(
        render_mesh_id,
        .tag = text128_make("[%s] instance_mesh_stagin", __func__),
        .arena = env.gpu_memory.frame_deferred_buffer[gpu->frame_index][buffer_memory_kind_coherent_src],
        .device = &gpu->device,
        .size = batch->instance_capacity
    );

    ent_session(entities)
    {
        for ent_each_with(query, static_geometry)
        {
            global_transform *gtr = query.global_transform; 
            render_mesh_id mesh_id = *query.render_mesh_id; 

            u32 local_geometry_id = ent_local_id_0_based(query.id, static_geometry);

            ((render_transform*)instance_transform_stagin->mem)[local_geometry_id].mat = gtr->mat;
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
    
}

void global_transform_handler_helper(ent_manager *manager, mat4 *parent_mat, ent_id id)
{           
    ent_session(manager)
    {        
        mat4_f32_mul_fast(
            parent_mat, 
            local_transform_as_mat(ent_get(id, local_transform)), 
            &ent_get(id, global_transform)->mat
        );
        
        for ent_each_child(ch, id)
        {            
            global_transform_handler_helper(manager, &ent_get(id, global_transform)->mat, *ch);
        }  
    }
}

thread_pool_task_declare(
    global_transform_propogation,
    { ent_manager *manager; ent_query query; },
    {
        ent_session(args->manager)
        {
            for ent_each_of(it, args->query)
            {
                if(!it.relation->parent)
                {
                    local_transform *ltr = it.local_transform;
                    
                    it.global_transform->mat = *local_transform_as_mat(it.local_transform);
                    
                    for ent_each_child(ch, it.id)
                    {
                        global_transform_handler_helper(args->manager, &it.global_transform->mat, *ch);                    
                    }                
                }
            }
        }
    }       
)

void global_transform_handler(ent_manager *manager, arena *temp_mem, thread_pool *pool)
{
    ent_session(manager)
    {
        ent_query query = ent_query_make(
            ent_get_session(), 
            ent_bitmask_make(local_transform, global_transform)
        ); 

        u32 count   = query.count;
        u32 workers = pool->size;
        u32 chunk   = (count + workers - 1) / workers;  

        for each_index(i, workers)
        {
            u32 start = i * chunk;
            if (start >= count) break;                   

            global_transform_propogation_args *arg = arena_ppush(temp_mem, global_transform_propogation_args);

            arg->manager = manager;
            arg->query = query;
            arg->query.entries       = query.entries + start;
            arg->query.count         = (count - start < chunk) ? (count - start) : chunk;
            arg->query.current_entry = 1;
            arg->query.id            = 0;
            ent_query_next(&arg->query); 

            thread_pool_schedule(
                pool, 
                global_transform_propogation_task_make(arg)
            );
        }

        thread_pool_wait_all(pool);
    }

}

void main_camera_update()
{
    if(env.gpu.swapchain_was_refreshed)
    {
        env.full_viewports[env.main_full_viewport_id].cam.ratio = (f32)env.gpu.swapchain.extent.width / (f32)env.gpu.swapchain.extent.height;        
    }

    f64 current_mouse_x, current_mouse_y;
    glfwGetCursorPos(env.win->handle, &current_mouse_x, &current_mouse_y);

    if (!env.not_first_mouse && env.mouse_captured)
    {
        env.last_mouse_x = current_mouse_x;
        env.last_mouse_y = current_mouse_y;
        env.not_first_mouse = true;
    }

    f64 delta_x = current_mouse_x - env.last_mouse_x;
    f64 delta_y = current_mouse_y - env.last_mouse_y;

    env.last_mouse_x = current_mouse_x;
    env.last_mouse_y = current_mouse_y;

    f32 mouse_sensitivity = 0.002f; 

    ent_session(&env.ent_manager)
    {
        for ent_each_with(it, camera_controller, global_transform)
        {                    
            if (env.mouse_captured)
            {                
                it.local_transform->rot.y -= (f32)delta_x * mouse_sensitivity; // yaw
                it.local_transform->rot.x -= (f32)delta_y * mouse_sensitivity; // pitch

                f32 max_pitch = 1.55f; 
                if (it.local_transform->rot.x > max_pitch) it.local_transform->rot.x = max_pitch;
                if (it.local_transform->rot.x < -max_pitch) it.local_transform->rot.x = -max_pitch;
                
                f32 velocity = (window_key_down(env.win, GLFW_KEY_LEFT_SHIFT) ? 30.0f : 15.0f) * env.tick_info.dt; 

                f32 yaw = it.local_transform->rot.y;
                
                f32 fwd_x = -sinf(yaw);
                f32 fwd_z = -cosf(yaw);
                
                f32 right_x = cosf(yaw);
                f32 right_z = -sinf(yaw);

                if (window_key_down(env.win, GLFW_KEY_W)) 
                {
                    it.local_transform->pos.x += fwd_x * velocity;
                    it.local_transform->pos.z += fwd_z * velocity;
                }
                if (window_key_down(env.win, GLFW_KEY_S)) 
                {
                    it.local_transform->pos.x -= fwd_x * velocity;
                    it.local_transform->pos.z -= fwd_z * velocity;
                }

                if (window_key_down(env.win, GLFW_KEY_A)) 
                {
                    it.local_transform->pos.x -= right_x * velocity;
                    it.local_transform->pos.z -= right_z * velocity;
                }
                if (window_key_down(env.win, GLFW_KEY_D)) 
                {
                    it.local_transform->pos.x += right_x * velocity;
                    it.local_transform->pos.z += right_z * velocity;
                }

                if (window_key_down(env.win, GLFW_KEY_SPACE)) 
                {
                    it.local_transform->pos.y += velocity; 
                }
                if (window_key_down(env.win, GLFW_KEY_LEFT_CONTROL)) 
                {
                    it.local_transform->pos.y -= velocity; 
                }
            }

            camara_update(
                &env.full_viewports[env.main_full_viewport_id].cam, 
                it.global_transform->mat
            ); 
        }
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
    ent_session(&env.ent_manager)
    {
        for ent_each_with(it, volatile_geometry)
        {            
            ent_id root = it.id;

            while(ent_get(root, relation)->parent)
            {
                root = ent_get(root, relation)->parent;
            }

            local_transform *ltr = ent_get(root, local_transform);

            f32 x = ltr->pos.x;
            f32 z = ltr->pos.z;
            
            f32 y = sinf((f32)env.tick_info.time * 2.0f + x * 0.05f + z * 0.05f) * 5.0f;

            local_transform_set_pos(ltr, (vec3){ .x = x, .y = y, .z = z });
        }
    }
}

void update()
{    
    env.exit = env.exit || window_should_close(env.win);

    tick_info_update(&env.tick_info);  

    profile_imm("move_stuff")
    {
        move_stuff();   
    }

    profile_imm("global_transform_handler")
    {
        global_transform_handler(&env.ent_manager, env.temp_memory, env.main_thread_pool);
    }
    
    main_camera_update();
}

void render_depth_only_viewport_record(VkCommandBuffer cmd, render_depth_only_viewport *vp)
{
    profile_imm("render_batch_fill_pass_record (depth_only vp = %x)", vp)
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

    profile_imm("depth_pass_record (depth_only vp = %x)", vp)    
    {
        depth_pass_record(
            cmd,
            &env.shared_gpu_passes.depth_pass,
            &env.gpu, 
            env.temp_memory,
            &env.main_render_manager.geometry,
            &vp->depth,
            render_geometry_type_count,
            vp->commands,
            &vp->cam
        );
    }
}

void render_full_viewport_record(VkCommandBuffer cmd, render_full_viewport *vp)
{
    profile_imm("render_batch_fill_pass_record (full vp = %x)", vp)
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

    profile_imm("depth_pass_record (full vp = %x)", vp)    
    {
        depth_pass_record(
            cmd,
            &env.shared_gpu_passes.depth_pass,
            &env.gpu, 
            env.temp_memory,
            &env.main_render_manager.geometry,
            &vp->depth,
            render_geometry_type_count,
            vp->commands,
            &vp->cam
        );
    }
    
    profile_imm("gbuffer_pass_record (full vp = %x)", vp)
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

    profile_imm("deferred_light_pass_record (full vp = %x)", vp)
    {
        deferred_light_pass_record(
            cmd,
            &vp->deferred_light_pass, 
            &env.gpu, 
            env.temp_memory, 
            env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            vp->depth,
            vp->color_target[render_color_target_albedo],
            vp->color_target[render_color_target_normal],
            vp->color_target[render_color_target_light],
            &vp->cam
        );
    }

    profile_imm("crt_pass_record (full vp = %x)", vp)
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
            env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            env.tick_info.time,
            vp->color_target[render_color_target_light],
            vp->color_target[render_color_target_crt],
            params
        );
    }
}

void present_record(VkCommandBuffer cmd, gpu_texture *texture[frames_in_flight])
{
    profile_imm("present_pass_record")
    {
        present_pass_record(
            cmd,
            &env.shared_gpu_passes.present_pass, 
            &env.gpu, 
            env.temp_memory,
            env.gpu_memory.scene_bound_buffer[buffer_memory_kind_coherent_resource_descriptor_buffer],
            texture
        );
    }    
}

void collect_geometry_record(VkCommandBuffer cmd)
{
    profile_imm("collect_static_geometry_record")
    {
        if(env.static_geometry_frames_to_collect)
        {
            collect_static_geometry_record(
                cmd,
                &env.gpu, 
                env.static_opaque_batches,
                &env.ent_manager
            );

            env.static_geometry_frames_to_collect--;
        }
    }

    profile_imm("collect_volatile_geometry")
    {
        collect_volatile_geometry(
            &env.gpu, 
            env.volatile_opaque_batches,
            &env.ent_manager
        );
    }
}

void render()
{   
    if (window_is_minimized(env.win)) { return; } 

    b8 begin_success = gpu_context_try_begin_frame(&env.gpu, env.win, env.temp_memory);

    for each_index(i, arrsize(frame_deferred_buffer_memory_entreis))
    {    
        buffer_memory_entry entry = frame_deferred_buffer_memory_entreis[i];
        gpu_buffer_arena_reset(env.gpu_memory.frame_deferred_buffer[env.gpu.frame_index][entry.kind], &env.gpu.device);
    }

    for each_index(i, arrsize(temp_buffer_memory_entries))
    {
        buffer_memory_entry entry = temp_buffer_memory_entries[i];
        gpu_buffer_arena_reset(env.gpu_memory.temp_buffer[entry.kind], &env.gpu.device);
    }

    if(begin_success)
    {
        if(env.gpu.swapchain_was_refreshed)
        {
            for each_index(i, arrsize(swapchain_bound_buffer_memory_entries))
            {
                buffer_memory_entry entry = swapchain_bound_buffer_memory_entries[i];
                gpu_buffer_arena_reset(env.gpu_memory.swapchain_bound_buffer[entry.kind], &env.gpu.device);
            }

            for each_index(i, arrsize(swapchain_bound_texture_memory_entries))
            {
                texture_memory_entry entry = swapchain_bound_texture_memory_entries[i];
                gpu_texture_arena_reset(env.gpu_memory.swapchain_bound_texture[entry.kind], &env.gpu.device);
            }

            refresh_swapchain_targets();
        }

        VkCommandBuffer cmd = gpu_context_grab_graphics(&env.gpu);
        
        collect_geometry_record(cmd);

        render_full_viewport_record(
            cmd, 
            &env.full_viewports[env.main_full_viewport_id]
        );

        present_record(
            cmd,
            env.full_viewports[env.main_full_viewport_id].color_target[render_color_target_crt]
        );

        vku_cmd_textures_transition_state(
            cmd,
            env.temp_memory, 
            { env.gpu.swapchain.textures + env.gpu.swapchain_image_index, gpu_texture_state_present }
        );
    
        gpu_context_commit_graphics(&env.gpu, cmd);
        
        profile_imm("gpu_context_end_frame")
        { 
            gpu_context_end_frame(&env.gpu, env.temp_memory);		
        }        
    }    
}

void jgfx_run(int argc, char **argv)
{
    log_dest(stdout);

    profile_imm("env_init") 
    {
        env_init(argc, argv);
    }        

	window_make_visible(env.win);
   
    while (!env.exit)
    {     
        profile_imm("frame %u", env.tick_info.tick)
        {
            events();
            update();
            render();
    
            msg("env.temp_memory current = %.2lf Kb", as_Kb(env.temp_memory->current));

            arena_reset(env.temp_memory); 
        }
    }
    
    profile_imm("env_release")
    {
        env_release();    
    }
}