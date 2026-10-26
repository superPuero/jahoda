#include "tick_info.h"
#include <jahoda/core/time.h>

void tick_info_update(tick_info *ti)
{
	ti->tick++;
	ti->dt = ti->time - ti->begin;
	ti->begin = ti->time;
	ti->time = (f64)jahoda_time_now();
}