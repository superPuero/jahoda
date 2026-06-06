#include "image.h"

#include <stb_image.h>

// @todo: rewrite stb_image to accets temp and static allocator, so there are no "wild" allocations and frees 
//        currently i can't think of way doing it both rapid and robus, it will just take some time when time comes

image image_load(arena *pf_arena, arena *storage_arena, strv path)
{
    image out = {0};
    str path_nt = str_from_view_nt(pf_arena, path);
    stbi_set_flip_vertically_on_load(true);
    out.data = stbi_load(path_nt.data, &out.extent.x, &out.extent.y, &out.channels, 4);    
    return out;
}

void image_release(image *image)
{
    stbi_image_free(image->data);    
}