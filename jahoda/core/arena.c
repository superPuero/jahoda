#include <assert.h>
#include <stdio.h>
#include "arena.h"

typedef struct
{
	char name[arena_name_max_len];
	uz current;
	uz capacity;
	u8 mem[];
} arena_content;

#define as_arena_content(...) ((arena_content*)(__VA_ARGS__))

arena arena_make_(arena_params params)
{
	arena_content* out = malloc(params.capacity + sizeof(arena_content));
	infol(alloc, "(%.*s) %.2lf Kb", strv_fmt(&params.name),  params.capacity / 1024.0);
	
	out->capacity = params.capacity;
	out->current = 0;

	verify(params.name.len < arena_name_max_len, "params.name.len < arena_name_max_len");

    memcpy(out->name, params.name.data, arena_name_max_len);

	return (arena)out;
}

void arena_release(arena arena)
{
	free(arena);
}

marker arena_mark(arena arena)
{
	return (marker){ .arena = arena, .point = as_arena_content(arena)->current };
}

void arena_pop_to_marker(marker marker)
{
	assert(as_arena_content(marker.arena)->current >= marker.point);
	as_arena_content(marker.arena)->current = marker.point;	
}

void *arena_push_unzeroed(arena arr, uz size, uz alignment)
{	
	arena_content *arena = as_arena_content(arr);
	ptr unaligned_current = (ptr)(arena->mem + arena->current);
	u32 align_offset = (u32)(((unaligned_current + (alignment - 1)) & ~(alignment - 1)) - unaligned_current);
	arena->current += align_offset;

	u8 *out = arena->mem + arena->current;
	arena->current += size;

	if(arena->current > arena->capacity)
	{
		err("arena [%s] overflow curr: %zu, capacity: %zu", arena->name, arena->current, arena->capacity);
		assert(0);
	}


	return out;
}

uz arena_current(arena arena)
{
	return as_arena_content(arena)->current; 
}

void arena_reset(arena arena)
{
	as_arena_content(arena)->current = 0;
}

void *arena_push(arena arena, uz size, uz alignment, bool8 zero)
{		
	u8 *out = arena_push_unzeroed(arena, size, alignment);
	if(zero) { memset(out, 0, size); }
	return out;
}


