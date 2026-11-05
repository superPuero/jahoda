#ifndef jahoda_camera
#define jahoda_camera

#include <jahoda/core/math.h>

typedef struct
{
    mat4 view;
    mat4 proj;
    mat4 proj_view;
    f32 ratio;
    f32 near;
    f32 far;
    f32 fov;
} camera;

void camara_update(camera *cam, mat4 transform);

#endif