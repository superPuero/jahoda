#include "arena.h"
#include "os.h"

arena *arena_make_(arena_params params)
{
	u64 page_size = os_fetch_hardware_info().page_size;

	u64 metadata_bytes = sizeof(arena);
	u64 total_pages_needed = (metadata_bytes + params.capacity + page_size - 1) / page_size;

	arena *out = os_mem_reserve(total_pages_needed * page_size);	

	u64 metadata_pages = (metadata_bytes + page_size - 1) / page_size;	
	os_mem_commit(out, metadata_pages * page_size, os_mem_flag_read_write);

	out->current = 0;
	out->name = params.name;
	out->capacity = params.capacity;
	out->page_count = total_pages_needed;
	out->page_size = page_size;
	out->commited_pages = metadata_pages;

	if(as_Tb(params.capacity) >= 1.0)
	{
		infol(cpu_mem, "[%s] reserved %.2lf Tb", out->name.data, as_Tb(params.capacity));
	}	
	else if(as_Gb(params.capacity) >= 1.0)
	{
		infol(cpu_mem, "[%s] reserved %.2lf Gb", out->name.data, as_Gb(params.capacity));
	}
	else if(as_Mb(params.capacity) >= 1.0)
	{
		infol(cpu_mem, "[%s] reserved %.2lf Mb", out->name.data, as_Mb(params.capacity));
	}
	else
	{
		infol(cpu_mem, "[%s] reserved %.2lf Kb", out->name.data, as_Kb(params.capacity));
	}

	return out;
}

b8 arena_is_at(arena *arena, void *ptr)
{
	return (void*)(arena->mem + arena->current) == ptr;
}

void arena_release(arena *arena)
{
	os_mem_release(arena, arena->page_count * arena->page_size);
}

scratch scratch_begin(arena *arena)
{
	return (scratch){ .arena = arena, .point = arena->current };
}

void scratch_end(scratch s)
{
	dbg_verify(s.arena->current >= s.point);
	s.arena->current = s.point;	
}

void *arena_push_bytes_unzeroed(arena *arena, u64 size, u64 alignment)
{		
	uptr unaligned_current = (uptr)(arena->mem + arena->current);
	u64 align_offset = (u64)(((unaligned_current + (alignment - 1)) & ~(alignment - 1)) - unaligned_current);
	u64 new_current = arena->current + align_offset + size;

	if(new_current > arena->capacity)
	{
		err("arena *[%s] overflow curr: %zu, capacity: %zu", arena->name, arena->current, arena->capacity);
	}

	u8* out = arena->mem + arena->current + align_offset;
	arena->current = new_current;

	u64 metadata_bytes = sizeof(*arena);
	u64 pages_crossed = ((metadata_bytes + arena->current + arena->page_size - 1) / arena->page_size);

	if(pages_crossed > arena->commited_pages)
	{
		os_mem_commit(arena_page(arena, arena->commited_pages), (pages_crossed - arena->commited_pages) * arena->page_size, os_mem_flag_read_write);
		arena->commited_pages = pages_crossed;
	}

	return out;
}

void *arena_page(arena *arena, u64 page)
{	
	return (u8*)(arena) + (page * arena->page_size);
}

u64 arena_current(arena *arena)
{
	return arena->current; 
}

u64 arena_reset(arena *arena)
{	
	u64 usage = arena->current;
	arena->current = 0;
	return usage;
}

void arena_reset_and_decommit(arena *arena)
{
	u64 metadata_bytes = sizeof(*arena);
	u64 metadata_pages = (metadata_bytes + arena->page_size - 1) / arena->page_size;

	u64 decommit_range = (arena->commited_pages - metadata_pages) * arena->page_size;
	
	if(decommit_range)
	{	
		os_mem_decommit(arena_page(arena, metadata_pages), decommit_range);
		arena->commited_pages = metadata_pages;
	}
	
	arena->current = 0;
}

void *arena_push_bytes(arena *arena, u64 size, u64 alignment, b8 zero)
{		
	u8 *out = arena_push_bytes_unzeroed(arena, size, alignment);
	if(zero) { memset(out, 0, size); }
	return out;
}

void arena_pop_to(arena *arena, u64 point)
{
	arena->current = point;
}

void arena_pop(arena *arena, u64 size)
{
	arena->current -= size;
}


