#ifndef jahoda_vertex
#define jahoda_vertex

#include <src/core/math.h>
 
#define basic_vertex_2d_list\
	XY(vec2_f32, position)\
	XY(vec2_f32, uv)\
	XY(vec4_f32, color)

typedef struct 
{
	#define XY(type, name)\
	type name;
	basic_vertex_2d_list
	#undef XY	
} basic_vertex_2d;

#endif