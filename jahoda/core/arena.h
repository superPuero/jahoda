#ifndef jahoda_arena
#define jahoda_arena

#include <stdlib.h>
#include "types.h"
#include "utils.h"

#define Kb(expr) (expr)  *1024ULL
#define Mb(expr) Kb(expr)  *1024ULL
#define Gb(expr) Mb(expr)  *1024ULL

#define arena_name_max_len 64

#define arena_page_size Kb(4)
#define arena_default_name "unnamed"

typedef u8* arena;

// typedef struct
// {
	// u8 *mem;
	// uz current;
	// uz capacity;
	// char name[arena_name_max_len];
// } arena;

typedef struct
{
	arena arena;
	uz point;
} scratch;

#define arena_ppush(arena, type) arena_push(arena, sizeof(type), jahoda_alignof(type), true)

#define arena_scope(arena, scope_name)\
scratch scope_name##scratch = scratch_begin(arena);\
for(i32 _i = 0; _i < 1; scratch_end(scope_name##scratch), ++_i)

typedef struct
{
	uz capacity;
	strv name;
} arena_params;

#define arena_make(...) arena_make_((arena_params){.name = arena_default_name, .capacity = arena_page_size, __VA_ARGS__})
arena arena_make_(arena_params params);

uz arena_current(arena arena);
void arena_reset(arena arena);
void arena_release(arena arena);
scratch scratch_begin(arena arena); 
void scratch_end(scratch); 
bool8 arena_is_at(arena arena, void *ptr);

void *arena_push(arena arena, uz size, uz alignment, bool8 zero);

#endif