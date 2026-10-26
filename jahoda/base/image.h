#ifndef jahoda_image
#define jahoda_image

#include <jahoda/core/core.h>

typedef struct
{
    u8 *data;
    i32 channels;
    vec2_i32 extent; 
} image;

image image_load(arena *storage_arena, str8 path);
image image_load_from_memory(arena *storage_arena, const u8 *buffer, u32 size);

void image_release(image *image);

#endif