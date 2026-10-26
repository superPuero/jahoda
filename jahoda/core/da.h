#ifndef jahoda_da
#define jahoda_da

#include "types.h"
#include "arena.h"

#define da_growth_coef 2
#define da_initial_capacity 8

#define da(data_type)\
struct\
{\
	data_type *data;\
	uz occupied;\
	uz capacity;\
}


#define sda(data_type, size)\
struct\
{\
	data_type data[size];\
	uz occupied;\
}

#define da_declare(data_type, da_type) typedef da(data_type) da_type;
#define sda_declare(data_type, size, da_type) typedef sda(data_type, size) da_type;

#define da_reserve_unchecked(arr, arena, amount)({\
	uz elem_size = sizeof((arr)->data[0]);\
	if(arena_is_at(arena, (arr)->data + (arr)->capacity))\
	{\
		arena_push_bytes(arena, (amount - (arr)->capacity) * elem_size, alignof((arr)->data[0]), true);\
		(arr)->capacity = amount;\
	}\
	else\
	{\
		(arr)->capacity = amount;\
		void *new_data = arena_push_bytes(arena, (arr)->capacity * elem_size, alignof((arr)->data[0]), true);\
		if((arr)->occupied) { memcpy(new_data, (arr)->data, elem_size  *(arr)->occupied); }\
		(arr)->data = new_data;\
	}\
})

#define da_reserve(arr, arena, amount)({\
	if((arr)->capacity < amount)\
	{\
		da_reserve_unchecked(arr, arena, amount);\
	}\
})
#define da_extend(arr, arena,  amount)\
do{\
	da_reserve(arr, arena, (arr)->capacity + amount);\
}while(0)

#define da_resize(arr, arena, amount)\
do{\
	da_reserve(arr, arena, amount);\
	(arr)->occupied = amount;\
}while(0)

#define da_append(arr, arena, ...)({\
	if((arr)->occupied == (arr)->capacity)\
	{\
		uz new_capacity = !(arr)->capacity ? da_initial_capacity : (arr)->capacity * da_growth_coef;\
		da_reserve_unchecked(arr, arena, new_capacity);\
	}\
	(arr)->data[(arr)->occupied] = (typeof(*(arr)->data))__VA_ARGS__;\
	(arr)->data + (arr)->occupied++;\
})

#define da_swap_and_pop(arr, index)({\
    (arr)->data[index] = (arr)->data[--(arr)->occupied];\
})

#define da_find(arr, val)({\
	u32 index = 0;\
    for da_each(it, arr)\
    {\
        if(*it == val)\
        {\
            break;\
        }\
        index++;\
    }\
	index;\
})

#define da_if_find(arr, val, out) for (u32 out = da_find(arr, val); out != (arr)->occupied;)

#define da_has(arr, val) (da_find(arr, val) != (arr)->occupied)

#define sda_append(arr, ...)({\
	dbg_verify(arrsize((arr)->data) != (arr)->occupied, "sda overflow");\
	(arr)->data[(arr)->occupied] = (typeof(*(arr)->data))__VA_ARGS__;\
	((typeof(*(arr)->data)*)(arr)->data) + (arr)->occupied++;\
})

#define da_last(arr) ((arr)->data + (arr)->occupied - 1)

#define da_each(itname, da)\
(typeof(*(da)->data)* itname = (da)->data; itname < ((typeof((da)->data[0])*)(da)->data) + (da)->occupied; itname++)

da_declare(const char*, cstr_da);

#endif