#include "hash.h"

u64 fnv1a_hash(hashee value)
{
	u64 out = fnv1a_offset_bias;

	for (uz i = 0; i < arrsize(value.bytes); ++i) 
	{
		out ^= value.bytes[i];
		out *= fnv1a_prime;
	}

	return out;
}

u64 fnv1a_hash_strv(strv strv)	
{
	u64 out = fnv1a_offset_bias;

	for (uz i = 0; i < strv.len; ++i) 
	{
		out ^= strv.data[i];
		out *= fnv1a_prime;
	}

	return out;
}
