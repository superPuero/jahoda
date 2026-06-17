#include <assert.h>
#include <stdio.h>
#include "arena.h"
#include "os.h"


arena *arena_make_(arena_params params)
{
	u64 metadata_bytes = sizeof(arena);
	u64 total_pages_needed = (metadata_bytes + params.capacity + os_page_size - 1) / os_page_size;

	arena *out = os_mem_reserve_pages(total_pages_needed);	

	u64 metadata_pages = (metadata_bytes + os_page_size - 1) / os_page_size;	
	os_mem_commit_pages(out, metadata_pages);

	verifyl(arena, params.name.len < arena_name_max_len);
	memcpy(out->name, params.name.data, arena_name_max_len);
	out->capacity = params.capacity;
	out->current = 0;
	out->commited_pages = metadata_pages;
	out->metadata_size = metadata_bytes;
	out->page_count = total_pages_needed;


	if(as_Tb(params.capacity) >= 1.0)
	{
		infol(alloc, "(%.*s) reserved %.2lf Tb", strv_fmt(&params.name), as_Tb(params.capacity));
	}	

	else if(as_Gb(params.capacity) >= 1.0)
	{
		infol(alloc, "(%.*s) reserved %.2lf Gb", strv_fmt(&params.name), as_Gb(params.capacity));
	}
	else if(as_Mb(params.capacity) >= 1.0)
	{
		infol(alloc, "(%.*s) reserved %.2lf Mb", strv_fmt(&params.name), as_Mb(params.capacity));
	}
	else
	{
		infol(alloc, "(%.*s) reserved %.2lf Kb", strv_fmt(&params.name), as_Kb(params.capacity));
	}

	return out;
}

bool8 arena_is_at(arena *arena, void *ptr)
{
	return (void*)(arena->mem + arena->current) == ptr;
}

void arena_release(arena *arena)
{
	os_mem_release_pages(arena, arena->page_count);
}

scratch scratch_begin(arena *arena)
{
	return (scratch){ .arena = arena, .point = arena->current };
}

void scratch_end(scratch scratch)
{
	assert(scratch.arena->current >= scratch.point);
	scratch.arena->current = scratch.point;	
}

void *arena_push_unzeroed(arena *arena, u64 size, u64 alignment)
{		
	ptr unaligned_current = (ptr)(arena->mem + arena->current);
	u64 align_offset = (u64)(((unaligned_current + (alignment - 1)) & ~(alignment - 1)) - unaligned_current);
	u64 new_current = arena->current + align_offset + size;

	if(new_current > arena->capacity)
	{
		err("arena *[%s] overflow curr: %zu, capacity: %zu", arena->name, arena->current, arena->capacity);
	}

	u8* out = arena->mem + arena->current + align_offset;
	arena->current = new_current;

	u64 pages_crossed = ((arena->metadata_size + arena->current + os_page_size - 1) / os_page_size);

	if(pages_crossed > arena->commited_pages)
	{
		os_mem_commit_pages(arena_page(arena, arena->commited_pages), pages_crossed - arena->commited_pages);
		arena->commited_pages = pages_crossed;
	}

	return out;
}

void *arena_page(arena *arena, u64 page)
{	
	return (u8*)(arena) + (page * os_page_size);
}

u64 arena_current(arena *arena)
{
	return arena->current; 
}

void arena_reset(arena *arena)
{	
	arena->current = 0;
}

void *arena_push(arena *arena, u64 size, u64 alignment, bool8 zero)
{		
	u8 *out = arena_push_unzeroed(arena, size, alignment);
	if(zero) { memset(out, 0, size); }
	return out;
}


