#include "utils.h"
#include "platform_detect.h"
#include "pthread.h"

#define jahoda_log_sync
 
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


void print_colored_if_isatty(const char* color, const char* str)
{
	if(jahoda_isatty(jahoda_fileno(get_log_dest())))
	{
		fprintf(get_log_dest(),
			ansi_esq_seq "[" "%s" "m" 
			"%s"
			ansi_esq_seq "[" ansi_text_color_default "m",
			color,
			str	
		);
	}
	else
	{
		fprintf(get_log_dest(), "%s",
			str
		);
	}
}

b8 is_whitespace(u8 ch)
{
	return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\v' || ch == 'f' || ch == '\r';
}

internal FILE *log_dest_file;

void log_dest(FILE *file)
{
	log_dest_file = file;
}
FILE *get_log_dest()
{
	return log_dest_file; 
}
