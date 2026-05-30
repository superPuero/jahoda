#include "utils.h"
#include "platform_detect.h"

strv strv_make(const char *data, uz len)
{
	return (strv){.data = data, .len = len};
}

strv strv_from_cstr(const char *data)
{
	return strv_make(data, strlen(data));
}
