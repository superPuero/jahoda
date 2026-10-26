#include "math.h"
#include <immintrin.h>
#include <jahoda/core/utils.h>

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
mat4_##dt mat4_##dt##_fill(dt v)\
{\
    mat4_##dt out; \
    out.x.x = v; \
    out.x.y = v; \
    out.x.z = v; \
    out.x.w = v; \
    out.y.x = v; \
    out.y.y = v; \
    out.y.z = v; \
    out.y.w = v; \
    out.z.x = v; \
    out.z.y = v; \
    out.z.z = v; \
    out.z.w = v; \
    out.w.x = v; \
    out.w.y = v; \
    out.w.z = v; \
    out.w.w = v; \
    return out;\
}\
dt mat4_##dt##_dot(mat4_##dt l, mat4_##dt r) { \
    return vec4_##dt##_dot(l.x, r.x) + vec4_##dt##_dot(l.y, r.y) + \
           vec4_##dt##_dot(l.z, r.z) + vec4_##dt##_dot(l.w, r.w); \
}\
mat4_##dt mat4_##dt##_translate(mat4_##dt m, vec3_##dt v) { \
    mat4_##dt res = m; \
    res.w.x = m.x.x * v.x + m.y.x * v.y + m.z.x * v.z + m.w.x; \
    res.w.y = m.x.y * v.x + m.y.y * v.y + m.z.y * v.z + m.w.y; \
    res.w.z = m.x.z * v.x + m.y.z * v.y + m.z.z * v.z + m.w.z; \
    res.w.w = m.x.w * v.x + m.y.w * v.y + m.z.w * v.z + m.w.w; \
    return res; \
} \
void mat4_##dt##_translate_ip(mat4_##dt *m, vec3_##dt v) { \
    m->w.x = m->x.x * v.x + m->y.x * v.y + m->z.x * v.z + m->w.x; \
    m->w.y = m->x.y * v.x + m->y.y * v.y + m->z.y * v.z + m->w.y; \
    m->w.z = m->x.z * v.x + m->y.z * v.y + m->z.z * v.z + m->w.z; \
    m->w.w = m->x.w * v.x + m->y.w * v.y + m->z.w * v.z + m->w.w; \
} \
mat4_##dt mat4_##dt##_scale(mat4_##dt m, vec3_##dt v) { \
    mat4_##dt res = m; \
    res.x.x = m.x.x * v.x; res.x.y = m.x.y * v.x; res.x.z = m.x.z * v.x; res.x.w = m.x.w * v.x; \
    res.y.x = m.y.x * v.y; res.y.y = m.y.y * v.y; res.y.z = m.y.z * v.y; res.y.w = m.y.w * v.y; \
    res.z.x = m.z.x * v.z; res.z.y = m.z.y * v.z; res.z.z = m.z.z * v.z; res.z.w = m.z.w * v.z; \
    return res; \
} \
void mat4_##dt##_scale_ip(mat4_##dt *m, vec3_##dt v) { \
    m->x.x = m->x.x * v.x; m->x.y = m->x.y * v.x; m->x.z = m->x.z * v.x; m->x.w = m->x.w * v.x; \
    m->y.x = m->y.x * v.y; m->y.y = m->y.y * v.y; m->y.z = m->y.z * v.y; m->y.w = m->y.w * v.y; \
    m->z.x = m->z.x * v.z; m->z.y = m->z.y * v.z; m->z.z = m->z.z * v.z; m->z.w = m->z.w * v.z; \
} \
mat4_##dt mat4_##dt##_rotate(mat4_##dt m, dt angle_radians, vec3_##dt axis) { \
    dt c = (dt)cos(angle_radians); \
    dt s = (dt)sin(angle_radians); \
    dt t = 1 - c; \
    dt len = (dt)sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z); \
    dt ax = axis.x / len; \
    dt ay = axis.y / len; \
    dt az = axis.z / len; \
    dt rot00 = ax * ax * t + c; \
    dt rot01 = ay * ax * t + az * s; \
    dt rot02 = az * ax * t - ay * s; \
    dt rot10 = ax * ay * t - az * s; \
    dt rot11 = ay * ay * t + c; \
    dt rot12 = az * ay * t + ax * s; \
    dt rot20 = ax * az * t + ay * s; \
    dt rot21 = ay * az * t - ax * s; \
    dt rot22 = az * az * t + c; \
    mat4_##dt res = m; \
    res.x.x = m.x.x * rot00 + m.y.x * rot01 + m.z.x * rot02; \
    res.x.y = m.x.y * rot00 + m.y.y * rot01 + m.z.y * rot02; \
    res.x.z = m.x.z * rot00 + m.y.z * rot01 + m.z.z * rot02; \
    res.x.w = m.x.w * rot00 + m.y.w * rot01 + m.z.w * rot02; \
    res.y.x = m.x.x * rot10 + m.y.x * rot11 + m.z.x * rot12; \
    res.y.y = m.x.y * rot10 + m.y.y * rot11 + m.z.y * rot12; \
    res.y.z = m.x.z * rot10 + m.y.z * rot11 + m.z.z * rot12; \
    res.y.w = m.x.w * rot10 + m.y.w * rot11 + m.z.w * rot12; \
    res.z.x = m.x.x * rot20 + m.y.x * rot21 + m.z.x * rot22; \
    res.z.y = m.x.y * rot20 + m.y.y * rot21 + m.z.y * rot22; \
    res.z.z = m.x.z * rot20 + m.y.z * rot21 + m.z.z * rot22; \
    res.z.w = m.x.w * rot20 + m.y.w * rot21 + m.z.w * rot22; \
    return res; \
} \
void mat4_##dt##_rotate_ip(mat4_##dt *m, dt angle_radians, vec3_##dt axis) { \
    *m = mat4_##dt##_rotate(*m, angle_radians, axis);\
} \
mat4_##dt mat4_##dt##_identity(dt val)\
{ \
    mat4_##dt m = {0};\
	m.x.x = (dt)val;\
	m.y.y = (dt)val;\
	m.z.z = (dt)val;\
	m.w.w = (dt)val;\
	return m;\
}\
mat4_##dt mat4_##dt##_inverse(mat4_##dt m) { \
    dt a00 = m.x.x, a01 = m.x.y, a02 = m.x.z, a03 = m.x.w; \
    dt a10 = m.y.x, a11 = m.y.y, a12 = m.y.z, a13 = m.y.w; \
    dt a20 = m.z.x, a21 = m.z.y, a22 = m.z.z, a23 = m.z.w; \
    dt a30 = m.w.x, a31 = m.w.y, a32 = m.w.z, a33 = m.w.w; \
    dt b00 = a00 * a11 - a01 * a10; \
    dt b01 = a00 * a12 - a02 * a10; \
    dt b02 = a00 * a13 - a03 * a10; \
    dt b03 = a01 * a12 - a02 * a11; \
    dt b04 = a01 * a13 - a03 * a11; \
    dt b05 = a02 * a13 - a03 * a12; \
    dt b06 = a20 * a31 - a21 * a30; \
    dt b07 = a20 * a32 - a22 * a30; \
    dt b08 = a20 * a33 - a23 * a30; \
    dt b09 = a21 * a32 - a22 * a31; \
    dt b10 = a21 * a33 - a23 * a31; \
    dt b11 = a22 * a33 - a23 * a32; \
    dt det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06; \
    if (det != (dt)0) { det = (dt)1 / det; } \
    mat4_##dt res; \
    res.x.x = (a11 * b11 - a12 * b10 + a13 * b09) * det; \
    res.x.y = (a02 * b10 - a01 * b11 - a03 * b09) * det; \
    res.x.z = (a31 * b05 - a32 * b04 + a33 * b03) * det; \
    res.x.w = (a22 * b04 - a21 * b05 - a23 * b03) * det; \
    res.y.x = (a12 * b08 - a10 * b11 - a13 * b07) * det; \
    res.y.y = (a00 * b11 - a02 * b08 + a03 * b07) * det; \
    res.y.z = (a32 * b02 - a30 * b05 - a33 * b01) * det; \
    res.y.w = (a20 * b05 - a22 * b02 + a23 * b01) * det; \
    res.z.x = (a10 * b10 - a11 * b08 + a13 * b06) * det; \
    res.z.y = (a01 * b08 - a00 * b10 - a03 * b06) * det; \
    res.z.z = (a30 * b04 - a31 * b02 + a33 * b00) * det; \
    res.z.w = (a21 * b02 - a20 * b04 - a23 * b00) * det; \
    res.w.x = (a11 * b07 - a10 * b09 - a12 * b06) * det; \
    res.w.y = (a00 * b09 - a01 * b07 + a02 * b06) * det; \
    res.w.z = (a31 * b01 - a30 * b03 - a32 * b00) * det; \
    res.w.w = (a20 * b03 - a21 * b01 + a22 * b00) * det; \
    return res; \
}
numeric_data_type_list
#undef X

void mat4_f32_mul_fast(const mat4_f32* l, const mat4_f32* r, mat4_f32* out) 
{
    __m256 l_x = _mm256_broadcast_ps((__m128*)&l->x.x);
    __m256 l_y = _mm256_broadcast_ps((__m128*)&l->y.x);
    __m256 l_z = _mm256_broadcast_ps((__m128*)&l->z.x);
    __m256 l_w = _mm256_broadcast_ps((__m128*)&l->w.x);

    for (int i = 0; i < 2; i++) 
    {
        f32* r_col = (f32*)r + (i * 8);

        __m256 r_x = _mm256_set_m128(_mm_set1_ps(r_col[4]), _mm_set1_ps(r_col[0]));
        __m256 r_y = _mm256_set_m128(_mm_set1_ps(r_col[5]), _mm_set1_ps(r_col[1]));
        __m256 r_z = _mm256_set_m128(_mm_set1_ps(r_col[6]), _mm_set1_ps(r_col[2]));
        __m256 r_w = _mm256_set_m128(_mm_set1_ps(r_col[7]), _mm_set1_ps(r_col[3]));

        __m256 res = _mm256_mul_ps(l_x, r_x);
        res = _mm256_fmadd_ps(l_y, r_y, res);
        res = _mm256_fmadd_ps(l_z, r_z, res);
        res = _mm256_fmadd_ps(l_w, r_w, res);

        _mm256_storeu_ps((f32*)out + (i * 8), res);
    }
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

mat4_f32 mat4_f32_proj(f32 fov_angle, f32 aspect_ratio, f32 near_z, f32 far_z)
{
    mat4_f32 m = {0}; 
    
    f32 radians = fov_angle * (jahoda_math_pi / 180.0f);

    f32 half_tan_fov = tanf(radians * 0.5f);
    
    m.x.x = 1.0f / (aspect_ratio * half_tan_fov);
    
    m.y.y = -1.0f / half_tan_fov; 
    
    m.z.z = far_z / (near_z - far_z);
    
    m.w.z = (near_z * far_z) / (near_z - far_z); 
    
    m.z.w = -1.0f;
    
    return m;
}