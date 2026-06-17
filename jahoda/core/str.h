// non-null terminated arena *based string
#ifndef jahoda_str
#define jahoda_str

#include <stdarg.h>
#include "types.h"
#include "utils.h"
#include "da.h"

da_declare(char, str);
da_declare(str, str_da);
da_declare(const char*, cstr_da);

#define str_fmt(str) (i32)(str)->occupied, (str)->data

#define str_put_fmt(arena, str, ...)\
{\
	da_resize((arena), (str), snprintf(NULL, 0, __VA_ARGS__));\
	snprintf((str)->data, (str)->occupied, __VA_ARGS__);\
}

strv strv_from_str(const str *str);

str str_from_cstr(arena *arena, const char *cstr);
str str_from_cstr_nt(arena *arena, const char *cstr);
str str_from_fmt(arena *arena, const char *fmt, ...);
str str_from_fmt_nt(arena *arena, const char *fmt, ...);
str str_from_fmt_va(arena *arena, const char *fmt, va_list list);

str str_from_view(arena *arena, strv view);
str str_from_view_nt(arena *arena, strv view);

str str_clone(arena *arena, const str *clonee);

void str_append_char(arena *arena, str *str, char ch);
void str_append_cstr(arena *arena, str *str, const char *cstr);
void str_append_str(arena *arena, str *to, str *what);
void str_append_view(arena *arena, str *str, strv view);

#endif

