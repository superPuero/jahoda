#ifndef jahoda_hash
#define jahoda_hash

#include "types.h"
#include "utils.h"

#define fnv1a_offset_bias 14695981039346656037ull
#define fnv1a_prime 1099511628211ull

typedef union 
{
	u64 value;
	u8 bytes[sizeof(u64)];
} hashee;

u64 fnv1a_hash(hashee value);
u64 fnv1a_hash_strv(strv strv);

#endif