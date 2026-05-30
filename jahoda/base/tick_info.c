#include "tick_info.h"
#include <jahoda/core/time.h>

void tick_info_update(tick_info *ti)
{
	ti->tick++;
	ti->dt = ti->end - ti->begin;
	ti->begin = ti->end;
	ti->end = jahoda_time_now();
}