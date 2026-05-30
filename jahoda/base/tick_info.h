#ifndef jahoda_tick_info
#define jahoda_tick_info

#include <jahoda/core/types.h>

typedef struct
{
	f64 dt;
	u64 tick;
	f64 begin;
	f64 end;
} tick_info;

void tick_info_update(tick_info *ti);

#endif
