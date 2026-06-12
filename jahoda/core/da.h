#ifndef jahoda_da
#define jahoda_da

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "arena.h"

// @perf: tweakin these migh be benefitial 
#define da_growth_coef 2
#define da_initial_capacity 8

#define da_declare(data_type, da_type)\
typedef struct\
{\
	data_type *it;\
	data_type *data;\
	uz occupied;\
	uz capacity;\
} da_type;

// @todo: use jahoda_typeof()

#define da_reserve(arena, arr, ammount)\
do{\
	if((arr)->capacity < ammount)\
	{\
		uz elem_size = sizeof((arr)->data[0]);\
		if(arena_is_at(arena, (arr)->data + (arr)->capacity))\
		{\
			arena_push(arena, (ammount - (arr)->capacity) * elem_size, jahoda_alignof((arr)->data[0]), true);\
			(arr)->capacity = ammount;\
		}\
		else\
		{\
			(arr)->capacity = ammount;\
			void *new_data = arena_push(arena, (arr)->capacity * elem_size, jahoda_alignof((arr)->data[0]), true);\
			if((arr)->occupied) { memcpy(new_data, (arr)->data, elem_size  *(arr)->occupied); }\
			(arr)->data = new_data;\
		}\
	}\
}while(0)

#define da_resize(arena, arr, ammount)\
do{\
	da_reserve(arena, arr, ammount);\
	(arr)->occupied = ammount;\
}while(0)


#define da_append(arena, arr, ...)\
do{\
	if((arr)->occupied == (arr)->capacity)\
	{\
		uz elem_size = sizeof((arr)->data[0]);\
		if(arena_is_at(arena, (arr)->data + (arr)->capacity))\
		{\
			uz new_capacity = !(arr)->capacity ? da_initial_capacity : (arr)->capacity * da_growth_coef;\
			arena_push(arena, (new_capacity - (arr)->capacity) * elem_size, jahoda_alignof((arr)->data[0]), true);\
			(arr)->capacity = new_capacity;\
		}\
		else\
		{\
			(arr)->capacity = !(arr)->capacity ? da_initial_capacity : (arr)->capacity * da_growth_coef;\
			void *new_data = arena_push(arena, (arr)->capacity  *elem_size, jahoda_alignof((arr)->data[0]), true);\
			if((arr)->occupied) { memcpy(new_data, (arr)->data, elem_size  *(arr)->occupied); }\
			(arr)->data = new_data;\
		}\
	}\
	(arr)->data[(arr)->occupied++] = __VA_ARGS__;\
}while(0)

#define da_last(arr) ((arr)->data + (arr)->occupied - 1)

#define da_foreach(da)\
for(uz _i = ((da)->it = NULL, 0); _i < (da)->occupied && ((da)->it = (da)->data + _i, 1); ++_i)

#define da_foreach_named(da, _it)\
for(uz _it = ((da)->it = NULL, 0); _it < (da)->occupied && ((da)->it = (da)->data + _it, 1); ++_it)

#endif