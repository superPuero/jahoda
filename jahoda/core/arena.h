#ifndef jahoda_arena
#define jahoda_arena

#include <stdlib.h>
#include "types.h"
#include "utils.h"

#define arena_name_max_len 64
#define arena_default_page_size Kb(4)
#define arena_default_name "unnamed"

typedef struct
{
	char name[arena_name_max_len + 1];
	u64 current;
	u64 metadata_size;
	u64 capacity;
	u64 page_count;
	u64 page_size;
	u64 commited_pages;
	u8 mem[];
} arena;

typedef struct
{
	arena *arena;
	u64 point;
} scratch;

#define arena_ppush(arena, type) arena_push(arena, sizeof(type), jahoda_alignof(type), true)

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
	strv name;
} arena_params;

arena *arena_make_(arena_params params);
#define arena_make(...) arena_make_((arena_params){ .name = arena_default_name, .capacity = arena_default_page_size, __VA_ARGS__})

u64 arena_current(arena *arena);
void arena_set_pages(arena *arena, u64 start_page, u64 pages);
void arena_reset(arena *arena);
void arena_release(arena *arena);
void *arena_page(arena *arena, u64 page);
scratch scratch_begin(arena *arena); 
void scratch_end(scratch); 
bool8 arena_is_at(arena *arena, void *ptr);

void *arena_push(arena *arena, u64 size, u64 alignment, bool8 zero);

#endif