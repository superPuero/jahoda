#ifndef jahoda_render_target_manager
#define jahoda_render_target_manager

#include "gpu_arena.h"

#define jahoda_win_sized_render_target_arena_mem Mb(128) 

typedef struct
{
	gpu_arena win_sized_render_targets_arena;

} render_target_manager;

render_target_manager render_target_manager_make(vk_device *device, vk_physical_device *pdevice);
void render_target_manager_release(vk_device *device, render_target_manager *manager);

#endif

