#include "str.h"


strv strv_from_str(const str *str)
{
	return (strv){.data = str->data, .len = str->occupied};
}

str str_from_fmt_va(arena arena, const char *fmt, va_list list)
{
    str out = {0};

    va_list list_copy;
    va_copy(list_copy, list);

	// @todo: vsnprintf might return -1
	da_resize(arena, &out, (uz)(vsnprintf(NULL, 0, fmt, list_copy) + 1));

    va_end(list_copy);

	vsnprintf(out.data, out.occupied, fmt, list);

	out.occupied--;

    return out;
}

str str_from_fmt(arena arena, const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);

    str out = str_from_fmt_va(arena, fmt, list);
    va_end(list);

    return out;
}

// @todo: str str_from_vfmt(arena arena, const char *fmt, va_list args)

str str_from_fmtnt(arena arena, const char *fmt, ...)
{
	str out = {0};

    va_list list;
    va_start(list, fmt);

    va_list list_copy;
    va_copy(list_copy, list);

	da_resize(arena, &out, (uz)(vsnprintf(NULL, 0, fmt, list_copy) + 1));

    va_end(list_copy);

	vsnprintf(out.data, out.occupied, fmt, list);

	// @explain: we dont append anything because null terminatr is already there from vsnprintf

    va_end(list);

	return out;
}
str str_from_cstr(arena arena, const char *cstr)
{
	str out = {0};
	str_append_view(arena, &out, strv_from_cstr(cstr));
	return out;
}

str str_from_cstr_nt(arena arena, const char *cstr)
{
	str out = str_from_cstr(arena, cstr);
	str_append_char(arena, &out, '\0');
	return out;
}

str str_from_view(arena arena, strv view)
{
	str out = {0};
	str_append_view(arena, &out, view);
	return out;
}

str str_from_view_nt(arena arena, strv view)
{
	str out = str_from_view(arena, view);
	str_append_char(arena, &out, '\0');
	return out;
}

str str_clone(arena arena, const str *clonee)
{
	return str_from_view(arena, strv_from_str(clonee));
}

void str_append_char(arena arena, str *str, char ch)
{
	da_append(arena, str, ch);
}

void str_append_cstr(arena arena, str *str, const char *cstr)
{
	str_append_view(arena, str, strv_from_cstr(cstr));
}

void str_append_str(arena arena, str *to, str *what)
{
	str_append_view(arena, to, strv_from_str(what));
}

void str_append_view(arena arena, str *str, strv view)
{
	// @memory: this might kill memory at some point
	for(uz i = 0; i < view.len; i++)
	{
		da_append(arena, str, view.data[i]);
	}
}