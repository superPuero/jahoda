#include <assert.h>
#include <stdio.h>
#include "arena.h"

arena arena_make_(arena_params params)
{
	arena arena = {.mem = malloc(params.capacity), .name = {0}, .current = 0 , .capacity = params.capacity};
	memcpy(arena.name, params.name.data, params.name.len);
	infol(alloc, "(%.*s) %.2lf Mb", strv_fmt(&params.name),  params.capacity / 1024.0 / 1024.0);
	return arena;
}

void arena_release(arena *arena)
{
	free(arena->mem);
}

marker arena_mark(arena *arena)
{
	return (marker){.arena = arena, .point = arena->current};
}

void arena_pop_to_marker(marker marker)
{
	assert(marker.arena->current >= marker.point);
	marker.arena->current = marker.point;	
}

void *arena_push_unzeroed(arena*arena, uz size, uz alignment)
{	
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

void arena_reset(arena *arena)
{
	arena->current = 0;
}

void *arena_push(arena*arena, uz size, uz alignment, bool8 zero)
{		
	u8 *out = arena_push_unzeroed(arena, size, alignment);
	if(zero) { memset(out, 0, size); }
	return out;
}


