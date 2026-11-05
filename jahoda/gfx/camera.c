#include "camera.h"

void camara_update(camera *cam, mat4 transform)
{
    cam->view = mat4_f32_inverse(transform);
    cam->proj = mat4_f32_proj(cam->fov, cam->ratio, cam->near, cam->far);
    mat4_f32_mul_fast(&cam->proj, &cam->view, &cam->proj_view);
}