#include "str.h"
#include "arena.h"

str8 str8_make_empty(arena *arena, u64 size)
{
	return (str8) { .data = arena_push_bytes(arena, size + 1, 1, 1), .size = size};
}

str8 str8_make(arena *arena, const char *fmt, ...)
{
  	va_list list;
    va_start(list, fmt);

    str8 out = str8_make_va(arena, fmt, list);
    va_end(list);

    return out;
}

str8 str8_make_va(arena *arena, const char *fmt, va_list list)
{
	str8 out = {0};

    va_list list_copy;
    va_copy(list_copy, list);
	
	out.size = vsnprintf(NULL, 0, fmt, list);
	out.data = arena_push_bytes(arena, out.size + 1, 1, 0);
	vsnprintf(out.data, out.size + 1, fmt, list);

	va_end(list_copy);

    return out;
}