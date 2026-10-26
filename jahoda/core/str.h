#ifndef jahoda_str
#define jahoda_str

#include <stdarg.h>
#include "types.h"
#include "utils.h"
#include "arena.h"

typedef struct 
{
	char *data;
	u64 size;
} str8;

#define str8_lit(lit) (str8){.data = lit, .size = sizeof(lit)}

#define str8_fmt "%.*s"
#define str8_arg(str) (i32)(str).size, (str).data

str8 str8_make_empty(arena *arena, u64 size);
str8 str8_make(arena *arena, const char *fmt, ...);
str8 str8_make_va(arena *arena, const char *fmt, va_list list);

#endif

