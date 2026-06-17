#ifndef jahoda_image
#define jahoda_image

#include <jahoda/core/core.h>

typedef struct
{
    u8 *data;
    i32 channels;
    vec2_i32 extent; 
} image;

image image_load(arena *pf_arena, arena *storage_arena, strv path);
void image_release(image *image);

#endif