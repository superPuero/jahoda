#include "transform.h"
#include <jahoda/core/utils.h>

mat4 *local_transform_as_mat(local_transform *lt)
{
    // if(lt->dirty_frames)
    {
        lt->mat = mat4_f32_identity(1.0f);
    
        mat4_f32_translate_ip(&lt->mat, lt->pos);
        
        vec3 axis_y = {0.0f, 1.0f, 0.0f};
        vec3 axis_x = {1.0f, 0.0f, 0.0f};
        vec3 axis_z = {0.0f, 0.0f, 1.0f};
    
        mat4_f32_rotate_ip(&lt->mat, lt->rot.y, axis_y);
        mat4_f32_rotate_ip(&lt->mat, lt->rot.x, axis_x);
        mat4_f32_rotate_ip(&lt->mat, lt->rot.z, axis_z);
    
        mat4_f32_scale_ip(&lt->mat, lt->scale);
    }

    return &lt->mat;
}

local_transform local_transform_from_mat(mat4 *mat)
{
    local_transform out = {.mat = *mat };
    
    out.pos.x = mat->w.x;
    out.pos.y = mat->w.y;
    out.pos.z = mat->w.z;

    out.scale.x = sqrtf(mat->x.x * mat->x.x + mat->x.y * mat->x.y + mat->x.z * mat->x.z);
    out.scale.y = sqrtf(mat->y.x * mat->y.x + mat->y.y * mat->y.y + mat->y.z * mat->y.z);
    out.scale.z = sqrtf(mat->z.x * mat->z.x + mat->z.y * mat->z.y + mat->z.z * mat->z.z);

    if (out.scale.x == 0.0f || out.scale.y == 0.0f || out.scale.z == 0.0f)
    {
        out.rot.x = 0.0f;
        out.rot.y = 0.0f;
        out.rot.z = 0.0f;
        return out;
    }

    f32 r00 = mat->x.x / out.scale.x;
    f32 r10 = mat->x.y / out.scale.x;
    f32 r20 = mat->x.z / out.scale.x;

    f32 r01 = mat->y.x / out.scale.y;
    f32 r11 = mat->y.y / out.scale.y;
    f32 r21 = mat->y.z / out.scale.y;

    f32 r02 = mat->z.x / out.scale.z;
    f32 r12 = mat->z.y / out.scale.z;
    f32 r22 = mat->z.z / out.scale.z;

    out.rot.y = asinf(-r20);

    if (cosf(out.rot.y) > 0.001f) 
    {
        out.rot.x = atan2f(r21, r22);
        out.rot.z = atan2f(r10, r00);
    } 
    else 
    {
        out.rot.x = atan2f(-r12, r11);
        out.rot.z = 0.0f;
    }
}

void local_transform_translate(local_transform *lt, vec3 vec)
{
    lt->pos  = vec3_f32_add(lt->pos, vec);
}

void local_transform_set_pos(local_transform *lt, vec3 vec)
{
    lt->pos  = vec;
}