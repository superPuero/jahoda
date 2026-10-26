#ifndef jahoda_hash
#define jahoda_hash

#include "types.h"
#include "utils.h"
#include "str.h"

#define fnv1a_offset_bias 14695981039346656037ull
#define fnv1a_prime 1099511628211ull

u64 fnv1a_hash_str8(str8 str);
u64 fnv1a_hash_memv(memv mem);

#endif