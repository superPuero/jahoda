#ifndef jahoda_transform
#define jahoda_transform

#include <jahoda/core/math.h>

typedef struct 
{
    vec3 pos;
    vec3 rot;
    vec3 scale;  
    mat4 mat;
} local_transform;

typedef struct 
{
    mat4 mat;
} global_transform;

mat4 *local_transform_as_mat(local_transform *lt);
local_transform local_transform_from_mat(mat4 *src);

void local_transform_translate(local_transform *lt, vec3 vec3);
void local_transform_set_pos(local_transform *lt, vec3 vec3);

#endif