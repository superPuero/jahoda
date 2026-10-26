#ifndef jahoda_profile
#define jahoda_profile

#include "types.h"
#include "da.h"

typedef struct
{
	text64 name;
    u64 process_cycles; 
    i64 start_ticks;
	f64 time_ms;
} profile_entry;

#define profile_fmt "[%s] %*s %.2lf ms, %llu cpuc"
#define profile_arg(prf) (prf)->name.data, (i32)(sizeof((prf)->name) - strlen((prf)->name.data) - 1), "",(prf)->time_ms, (prf)->process_cycles

profile_entry profile_begin(text64 name);
void profile_end(profile_entry *prf);

da_declare(profile_entry, profile_da);


#define profile(arena, da, ...)\
    for(profile_entry *_prf = da_append(arena, da, profile_begin(arena, text32_make(__VA_ARGS__))); _prf.start_ticks; profile_end(_prf))

#define profile_imm(...)\
    for(profile_entry _prf = profile_begin(text64_make(__VA_ARGS__)); (_prf.start_ticks || (msgl(profile, profile_fmt, profile_arg(&_prf)), 0)); profile_end(&_prf))


#endif