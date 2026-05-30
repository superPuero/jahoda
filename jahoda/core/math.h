#ifndef jahoda_math
#define jahoda_math

#include <math.h>
#include "types.h"

#define jahoda_math_epsilon      1.19209290e-7f
#define jahoda_math_zero         0.0f
#define jahoda_math_one          1.0f
#define jahoda_math_two_thirds   0.666666666666666666666666666666666666667f

#define jahoda_math_tau          6.28318530717958647692528676655900576f
#define jahoda_math_pi           3.14159265358979323846264338327950288f
#define jahoda_math_one_over_tau 0.159154943091895335768883763372514362f
#define jahoda_math_one_over_py  0.318309886183790671537767526745028724f

#define jahoda_math_tau_over_2   3.14159265358979323846264338327950288f
#define jahoda_math_tau_over_4   1.570796326794896619231321691639751442f
#define jahoda_math_tau_over_8   0.785398163397448309615660845819875721f

#define jahoda_math_e            2.7182818284590452353602874713526625f
#define jahoda_math_sqrt_two     1.41421356237309504880168872420969808f
#define jahoda_math_sqrt_three   1.73205080756887729352744634150587236f
#define jahoda_math_sqrt_five    2.23606797749978969640917366873127623f

#define jahoda_math_log_two      0.693147180559945309417232121458176568f
#define jahoda_math_log_ten      2.30258509299404568401799145468436421f

#define jh_abs(x) ((x) > 0 ? (x) : -(x))
#define jh_sign(x) ((x) >= 0 ? 1 : -1)

#define vec2_len_sq(vec) (((vec).x  *(vec).x) + ((vec).y  *(vec).y))
#define vec2_add(to, ...) (((to).x += (__VA_ARGS__).x), ((to).y += (__VA_ARGS__).y), to)
#define vec2_sub(to, ...) (((to).x -= (__VA_ARGS__).x), ((to).y -= (__VA_ARGS__).y), to)
#define vec2_mul(to, ...) (((to).x *= (__VA_ARGS__).x), ((to).y *= (__VA_ARGS__).y), to)

#define numeric_data_type_list\
	X(i8)\
	X(u8)\
	X(i16)\
	X(u16)\
	X(i32)\
	X(u32)\
	X(i64)\
	X(u64)\
	X(f64)\
	X(f32)

#define vec2_cast(to, value) (vec2_##to){.x = (value).x, .y = (value).y}
#define vec3_cast(to, value) (vec3_##to){.x = (value).x, .y = (value).y, .z = (value).z}
#define vec4_cast(to, value) (vec4_##to){.x = (value).x, .y = (value).y, .z = (value).z, .w = (value).w}

#define X(dt)\
typedef struct\
{\
	dt x;\
	dt y;\
} vec2_##dt;\
vec2_##dt 	vec2_##dt##_add(vec2_##dt l, vec2_##dt r);\
vec2_##dt 	vec2_##dt##_sub(vec2_##dt l, vec2_##dt r);\
vec2_##dt 	vec2_##dt##_mul(vec2_##dt l, vec2_##dt r);\
vec2_##dt 	vec2_##dt##_div(vec2_##dt l, vec2_##dt r);\
dt 			vec2_##dt##_dot(vec2_##dt l, vec2_##dt r);
numeric_data_type_list
#undef X

typedef vec2_f64 vec2;

#define X(dt)\
typedef struct\
{\
	dt x;\
	dt y;\
	dt z;\
} vec3_##dt;\
vec3_##dt 	vec3_##dt##_add(vec3_##dt l, vec3_##dt r);\
vec3_##dt 	vec3_##dt##_sub(vec3_##dt l, vec3_##dt r);\
vec3_##dt 	vec3_##dt##_mul(vec3_##dt l, vec3_##dt r);\
vec3_##dt 	vec3_##dt##_div(vec3_##dt l, vec3_##dt r);\
dt 			vec3_##dt##_dot(vec3_##dt l, vec3_##dt r);
numeric_data_type_list
#undef X

#define X(dt)\
typedef struct\
{\
	dt x;\
	dt y;\
	dt z;\
	dt w;\
} vec4_##dt;\
vec4_##dt 	vec4_##dt##_add(vec4_##dt l, vec4_##dt r);\
vec4_##dt 	vec4_##dt##_sub(vec4_##dt l, vec4_##dt r);\
vec4_##dt 	vec4_##dt##_mul(vec4_##dt l, vec4_##dt r);\
vec4_##dt 	vec4_##dt##_div(vec4_##dt l, vec4_##dt r);\
dt 			vec4_##dt##_dot(vec4_##dt l, vec4_##dt r);
numeric_data_type_list
#undef X

// column major
#define X(dt)\
typedef struct\
{\
	vec4_##dt x;\
	vec4_##dt y;\
	vec4_##dt z;\
	vec4_##dt w;\
} mat4_##dt;\
mat4_##dt 	mat4_##dt##_add(mat4_##dt l, mat4_##dt r);\
mat4_##dt 	mat4_##dt##_sub(mat4_##dt l, mat4_##dt r);\
mat4_##dt 	mat4_##dt##_mul(mat4_##dt l, mat4_##dt r);\
mat4_##dt 	mat4_##dt##_div(mat4_##dt l, mat4_##dt r);\
dt 			mat4_##dt##_dot(mat4_##dt l, mat4_##dt r);
numeric_data_type_list
#undef X

typedef struct
{
	f32 left;
	f32 right;
	f32 bottom;
	f32 top;
	f32 near_z;
	f32 far_z;
} mat4_f32_ortho_params;

#define mat4_f32_ortho(...) mat4_f32_ortho_((mat4_f32_ortho_params){__VA_ARGS__})

mat4_f32 mat4_f32_identity();
mat4_f32 mat4_f32_ortho_(mat4_f32_ortho_params params);
mat4_f32 mat4_f32_proj(f32 fov_y_radians, f32 aspect_ratio, f32 near_z, f32 far_z);
#endif