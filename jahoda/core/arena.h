#ifndef jahoda_arena
#define jahoda_arena

#include "types.h"
#include "utils.h"

#define arena_name_max_len 63
#define arena_default_capacity Gb(1)
#define arena_default_name "unnamed"

typedef struct arena arena;

struct arena
{
	text64 name;
	u64 current;
	u64 capacity;
	u64 page_count;
	u64 page_size;
	u64 commited_pages; // includes metadata page
	u8 mem[];
};

typedef struct
{
	arena *arena;
	u64 point;
} scratch;

#define arena_ppush_arr(arena, type, size) arena_push_bytes(arena, sizeof(type) * size, alignof(type), true)
#define arena_ppush(arena, type) arena_ppush_arr(arena, type, 1)
#define arena_fpush_arr(arena, type, size) arena_push_bytes(arena, sizeof(type) * size, alignof(type), false)
#define arena_fpush(arena, type) arena_fpush_arr(arena, type, 1)
#define arena_push(arena, ...) ({ \
    typeof(__VA_ARGS__)* _ptr = (typeof(__VA_ARGS__)*)arena_fpush(arena, typeof(__VA_ARGS__)); \
    *_ptr = (typeof(__VA_ARGS__))__VA_ARGS__; \
    _ptr; \
})

#define arena_scope(arena, scope_name)\
scratch scope_name##scratch = scratch_begin(arena);\
for(i32 _i = 0; _i < 1; scratch_end(scope_name##scratch), ++_i)

typedef u64 arena_flags;

typedef enum
{
	arena_flag_imm_commit = 1 << 0
} arena_flags_;

typedef struct
{
	arena_flags flags;
	u64 capacity;
	text64 name;
} arena_params;

arena *arena_make_(arena_params params);
#define arena_make(...) arena_make_((arena_params){ __VA_ARGS__ })

u64 arena_current(arena *arena);
void arena_pop_to(arena *arena, u64 point);
void arena_set_pages(arena *arena, u64 start_page, u64 pages);
u64 arena_reset(arena *arena);
void arena_reset_and_decommit(arena *arena);
void arena_release(arena *arena);
void *arena_page(arena *arena, u64 page);
scratch scratch_begin(arena *arena); 
void scratch_end(scratch s); 
b8 arena_is_at(arena *arena, void *ptr);
void *arena_push_bytes(arena *arena, u64 size, u64 alignment, b8 zero);
void arena_pop(arena *arena, u64 size);
	

#define pool_declare(type, pool_type)\
typedef struct\
{\
	type *mem;\
	u32 occupied;\
	u32 capacity;\
	u32 free_list_occupied;\
	u32 *free_list;\
} pool_type;\
static inline pool_type pool_type##_make(arena *mem, u32 capacity)\
{\
	return (pool_type) {\
		.mem = arena_fpush_arr(mem, type, capacity),\
		.free_list = arena_fpush_arr(mem, u32, capacity),\
		.capacity = capacity\
	};\
}\
static inline type *pool_type##_insert(pool_type *pool)\
{\
	if(pool->free_list_occupied)\
	{\
		return pool->mem + pool->free_list[--pool->free_list_occupied];\
	}\
	return pool->mem + pool->occupied++;\
}\
static inline void pool_type##_remove(pool_type *pool, type *value)\
{\
	pool->free_list[pool->free_list_occupied++] = (u32)(value - pool->mem);\
}



#endif