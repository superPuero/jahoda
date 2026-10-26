#include "image.h"

#include <stb_image.h>

// @todo: rewrite stb_image to access temp and static allocator, so there are no "wild" allocations and frees 
//        currently i can't think of way doing it both rapid and robus, it will just take some time when time comes

image image_load(arena *storage_arena, str8 path)
{
    image out = {0};
    stbi_set_flip_vertically_on_load(true);
    out.data = stbi_load(path.data, &out.extent.x, &out.extent.y, &out.channels, 4);    
    return out;
}

image image_load_from_memory(arena *storage_arena, const u8 *buffer, u32 size)
{
    image out = {0};
    stbi_set_flip_vertically_on_load(true);
    out.data = stbi_load_from_memory(buffer, size, &out.extent.x, &out.extent.y, &out.channels, 4);    
    return out;
}

void image_release(image *image)
{
    stbi_image_free(image->data);    
}