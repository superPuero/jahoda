#ifndef jahoda_da
#define jahoda_da

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "arena.h"

// @perf: tweakin these migh be benefitial 
#define da_growth_coef 2
#define da_initial_capacity 8

#define da(data_type)\
struct\
{\
	data_type *data;\
	uz occupied;\
	uz capacity;\
}

#define da_declare(data_type, da_type) typedef da(data_type) da_type;

// @todo: use jahoda_typeof()

#define da_reserve_unchecked(arena, arr, amount)\
do{\
	uz elem_size = sizeof((arr)->data[0]);\
	if(arena_is_at(arena, (arr)->data + (arr)->capacity))\
	{\
		arena_push(arena, (amount - (arr)->capacity) * elem_size, jahoda_alignof((arr)->data[0]), true);\
		(arr)->capacity = amount;\
	}\
	else\
	{\
		(arr)->capacity = amount;\
		void *new_data = arena_push(arena, (arr)->capacity * elem_size, jahoda_alignof((arr)->data[0]), true);\
		if((arr)->occupied) { memcpy(new_data, (arr)->data, elem_size  *(arr)->occupied); }\
		(arr)->data = new_data;\
	}\
}while(0)

#define da_reserve(arena, arr, amount)\
do{\
	if((arr)->capacity < amount)\
	{\
		da_reserve_unchecked(arena, arr, amount);\
	}\
}while(0)

#define da_extend(arena, arr, amount)\
do{\
	da_reserve(arena, arr, (arr)->capacity + amount);\
}while(0)

#define da_resize(arena, arr, amount)\
do{\
	da_reserve(arena, arr, amount);\
	(arr)->occupied = amount;\
}while(0)

#define da_append(arena, arr, ...)\
do{\
	if((arr)->occupied == (arr)->capacity)\
	{\
		uz new_capacity = !(arr)->capacity ? da_initial_capacity : (arr)->capacity * da_growth_coef;\
		da_reserve_unchecked(arena, arr, new_capacity);\
	}\
	(arr)->data[(arr)->occupied++] = __VA_ARGS__;\
}while(0)

#define da_last(arr) ((arr)->data + (arr)->occupied - 1)

#define da_each(da, itname)\
(jahoda_typeof((da)->data) itname = (da)->data; itname < (da)->data + (da)->occupied; itname++)


#endif