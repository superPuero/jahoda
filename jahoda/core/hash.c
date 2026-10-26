#include "hash.h"

u64 fnv1a_hash_str8(str8 str)
{
	u64 out = fnv1a_offset_bias;

	for (uz i = 0; i < str.size; ++i) 
	{
		out ^= str.data[i];
		out *= fnv1a_prime;
	}

	return out;
}

u64 fnv1a_hash_memv(memv mem)
{
	u64 out = fnv1a_offset_bias;

	for (uz i = 0; i < mem.size; ++i) 
	{
		out ^= mem.data[i];
		out *= fnv1a_prime;
	}

	return out;
}