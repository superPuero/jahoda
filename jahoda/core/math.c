#include "math.h"

// VEC2 IMPLEMENTATION
#define X(dt)\
vec2_##dt vec2_##dt##_add(vec2_##dt l, vec2_##dt r) { return (vec2_##dt){l.x + r.x, l.y + r.y}; }\
vec2_##dt vec2_##dt##_sub(vec2_##dt l, vec2_##dt r) { return (vec2_##dt){l.x - r.x, l.y - r.y}; }\
vec2_##dt vec2_##dt##_mul(vec2_##dt l, vec2_##dt r) { return (vec2_##dt){l.x * r.x, l.y * r.y}; }\
vec2_##dt vec2_##dt##_div(vec2_##dt l, vec2_##dt r) { return (vec2_##dt){l.x / r.x, l.y / r.y}; }\
dt        vec2_##dt##_dot(vec2_##dt l, vec2_##dt r) { return (l.x * r.x) + (l.y * r.y); }
numeric_data_type_list
#undef X

// VEC3 IMPLEMENTATION
#define X(dt)\
vec3_##dt vec3_##dt##_add(vec3_##dt l, vec3_##dt r) { return (vec3_##dt){l.x + r.x, l.y + r.y, l.z + r.z}; }\
vec3_##dt vec3_##dt##_sub(vec3_##dt l, vec3_##dt r) { return (vec3_##dt){l.x - r.x, l.y - r.y, l.z - r.z}; }\
vec3_##dt vec3_##dt##_mul(vec3_##dt l, vec3_##dt r) { return (vec3_##dt){l.x * r.x, l.y * r.y, l.z * r.z}; }\
vec3_##dt vec3_##dt##_div(vec3_##dt l, vec3_##dt r) { return (vec3_##dt){l.x / r.x, l.y / r.y, l.z / r.z}; }\
dt        vec3_##dt##_dot(vec3_##dt l, vec3_##dt r) { return (l.x * r.x) + (l.y * r.y) + (l.z * r.z); }
numeric_data_type_list
#undef X

// VEC4 IMPLEMENTATION
#define X(dt)\
vec4_##dt vec4_##dt##_add(vec4_##dt l, vec4_##dt r) { return (vec4_##dt){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }\
vec4_##dt vec4_##dt##_sub(vec4_##dt l, vec4_##dt r) { return (vec4_##dt){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }\
vec4_##dt vec4_##dt##_mul(vec4_##dt l, vec4_##dt r) { return (vec4_##dt){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }\
vec4_##dt vec4_##dt##_div(vec4_##dt l, vec4_##dt r) { return (vec4_##dt){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }\
dt        vec4_##dt##_dot(vec4_##dt l, vec4_##dt r) { return (l.x * r.x) + (l.y * r.y) + (l.z * r.z) + (l.w * r.w); }
numeric_data_type_list
#undef X

// MAT4 IMPLEMENTATION
#define X(dt)\
mat4_##dt mat4_##dt##_add(mat4_##dt l, mat4_##dt r) { \
    return (mat4_##dt){ \
        vec4_##dt##_add(l.x, r.x), vec4_##dt##_add(l.y, r.y), \
        vec4_##dt##_add(l.z, r.z), vec4_##dt##_add(l.w, r.w) \
    }; \
}\
mat4_##dt mat4_##dt##_sub(mat4_##dt l, mat4_##dt r) { \
    return (mat4_##dt){ \
        vec4_##dt##_sub(l.x, r.x), vec4_##dt##_sub(l.y, r.y), \
        vec4_##dt##_sub(l.z, r.z), vec4_##dt##_sub(l.w, r.w) \
    }; \
}\
mat4_##dt mat4_##dt##_div(mat4_##dt l, mat4_##dt r) { \
    return (mat4_##dt){ \
        vec4_##dt##_div(l.x, r.x), vec4_##dt##_div(l.y, r.y), \
        vec4_##dt##_div(l.z, r.z), vec4_##dt##_div(l.w, r.w) \
    }; \
}\
mat4_##dt mat4_##dt##_mul(mat4_##dt l, mat4_##dt r) { \
    mat4_##dt out; \
    out.x.x = l.x.x * r.x.x + l.y.x * r.x.y + l.z.x * r.x.z + l.w.x * r.x.w; \
    out.x.y = l.x.y * r.x.x + l.y.y * r.x.y + l.z.y * r.x.z + l.w.y * r.x.w; \
    out.x.z = l.x.z * r.x.x + l.y.z * r.x.y + l.z.z * r.x.z + l.w.z * r.x.w; \
    out.x.w = l.x.w * r.x.x + l.y.w * r.x.y + l.z.w * r.x.z + l.w.w * r.x.w; \
    out.y.x = l.x.x * r.y.x + l.y.x * r.y.y + l.z.x * r.y.z + l.w.x * r.y.w; \
    out.y.y = l.x.y * r.y.x + l.y.y * r.y.y + l.z.y * r.y.z + l.w.y * r.y.w; \
    out.y.z = l.x.z * r.y.x + l.y.z * r.y.y + l.z.z * r.y.z + l.w.z * r.y.w; \
    out.y.w = l.x.w * r.y.x + l.y.w * r.y.y + l.z.w * r.y.z + l.w.w * r.y.w; \
    out.z.x = l.x.x * r.z.x + l.y.x * r.z.y + l.z.x * r.z.z + l.w.x * r.z.w; \
    out.z.y = l.x.y * r.z.x + l.y.y * r.z.y + l.z.y * r.z.z + l.w.y * r.z.w; \
    out.z.z = l.x.z * r.z.x + l.y.z * r.z.y + l.z.z * r.z.z + l.w.z * r.z.w; \
    out.z.w = l.x.w * r.z.x + l.y.w * r.z.y + l.z.w * r.z.z + l.w.w * r.z.w; \
    out.w.x = l.x.x * r.w.x + l.y.x * r.w.y + l.z.x * r.w.z + l.w.x * r.w.w; \
    out.w.y = l.x.y * r.w.x + l.y.y * r.w.y + l.z.y * r.w.z + l.w.y * r.w.w; \
    out.w.z = l.x.z * r.w.x + l.y.z * r.w.y + l.z.z * r.w.z + l.w.z * r.w.w; \
    out.w.w = l.x.w * r.w.x + l.y.w * r.w.y + l.z.w * r.w.z + l.w.w * r.w.w; \
    return out; \
}\
dt mat4_##dt##_dot(mat4_##dt l, mat4_##dt r) { \
    return vec4_##dt##_dot(l.x, r.x) + vec4_##dt##_dot(l.y, r.y) + \
           vec4_##dt##_dot(l.z, r.z) + vec4_##dt##_dot(l.w, r.w); \
}
numeric_data_type_list
#undef X

mat4_f32 mat4_f32_identity()
{
    mat4_f32 m = {0}; 
	m.x.x = 1;
	m.y.y = 1;
	m.z.z = 1;
	m.w.w = 1;
	return m;
}


mat4_f32 mat4_f32_ortho_(mat4_f32_ortho_params params)
{
    mat4_f32 m = {0}; 
    
    m.x.x = 2.0f / (params.right - params.left);
    m.y.y = 2.0f / (params.bottom - params.top);
    m.z.z = 1.0f / (params.far_z - params.near_z);
    m.w.w = 1.0f;
    
    m.w.x = -(params.right + params.left) / (params.right - params.left);
    m.w.y = -(params.bottom + params.top) / (params.bottom - params.top);
    m.w.z = -params.near_z / (params.far_z - params.near_z);
    
    return m;
}

mat4_f32 mat4_f32_proj(f32 fov_y_radians, f32 aspect_ratio, f32 near_z, f32 far_z)
{
    mat4_f32 m = {0}; 
    
    f32 half_tan_fov = tanf(fov_y_radians * 0.5f);
    
    m.x.x = 1.0f / (aspect_ratio * half_tan_fov);
    
    m.y.y = -1.0f / half_tan_fov; 
    
    m.z.z = far_z / (near_z - far_z);
    
    m.w.z = (near_z * far_z) / (near_z - far_z); 
    
    m.z.w = -1.0f;
    
    return m;
}