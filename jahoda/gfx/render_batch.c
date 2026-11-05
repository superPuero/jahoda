// #include "render_batch.h"

// render_batch_set render_batch_set_make(render_manager *manager, gpu_context *gpu, arena *mem, u32 instance_count)
// {
//     render_batch_set out;

//     for each_index(i, frames_in_flight)
//     {
//         out.entries[i] = (render_batch){
//             .instance_counts = arena_fpush_arr(mem, u32, manager->mesh_capacity),
//             .mesh_id_buffer = arena_fpush_arr(mem, render_mesh_id, instance_count),
//             .transform_buffer = arena_push_bytes(mem, instance_count * sizeof(mat4), 16, false), // alignas(16)
//         };
//     }

//     return out;
// }

