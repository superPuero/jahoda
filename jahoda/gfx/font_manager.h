#ifndef jahoda_font_manager
#define jahoda_font_manager

#include <jahoda/base/font.h>
#include "gpu_arena.h"
#include "gpu_context.h"
#include "texture.h"

#define jahoda_font_manager_gpu_memory_size Mb(8) 
#define jahoda_max_font_count 4

typedef u32 font_id;

typedef struct
{
	font_atlas altas;
	texture tex;
} font_manager_entry;

typedef struct
{
	u32 next_font_id;
	font_manager_entry entries[jahoda_max_font_count];
} font_manager;

font_id font_manager_load(font_manager *manager, gpu_context *gpu, arena *static_arena, arena *scratch_arena, strv font_path);
void font_manager_release(font_manager *manager, gpu_context *gpu);

#endif