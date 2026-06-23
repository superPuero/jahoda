#include "utils.h"
#include "platform_detect.h"
#include "pthread.h"

#ifdef jahoda_log_sync

internal pthread_mutex_t log_sync_mutex = PTHREAD_MUTEX_INITIALIZER; 

void log_sync_lock()
{
	pthread_mutex_lock(&log_sync_mutex);
}

void log_sync_unlock()
{
	pthread_mutex_unlock(&log_sync_mutex);
}

#else
void log_sync_lock() {}
void log_sync_unlock() {}
#endif

strv strv_make(const char *data, uz len)
{
	return (strv){.data = data, .len = len};
}

strv strv_from_cstr(const char *data)
{
	return strv_make(data, strlen(data));
}

bool8 strv_compare(strv strv1, strv strv2)
{
	if(strv1.len != strv2.len)
	{
		return false;
	}

	return strncmp(strv1.data, strv2.data, strv2.len) == 0;
}
