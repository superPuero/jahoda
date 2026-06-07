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

typedef struct uz arena_marker;

typedef struct
{
	arena arena;
	uz point;
} marker;

#define arena_ppush(arena, type) arena_push(arena, sizeof(type), jahoda_alignof(type), true)

#define arena_scope(arena, scope_name)\
marker scope_name##marker = arena_mark(arena);\
for(i32 _i = 0; _i < 1; arena_pop_to_marker(scope_name##marker), ++_i)

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
marker arena_mark(arena arena); 
void arena_pop_to_marker(marker); 


void *arena_push(arena arena, uz size, uz alignment, bool8 zero);

#endif