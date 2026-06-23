#ifndef jahoda_ht
#define jahoda_ht

#include "da.h"
#include "hash.h"

#define ht_tombstone 0
#define ht_tombstone_replacement 0xdeadbeef

#define ht_max_occupancy 0.75

da_declare(u64, ht_key_da);

#define ht_declare(data_type, size_power_of_two, ht_name)\
typedef struct\
{\
	data_type values[(1 << size_power_of_two)];\
	u64 keys[(1 << size_power_of_two)];\
	u64 occupied;\
} ht_name;

#define ht_cap(table) arrsize((table)->values)

#define ht_insert(table, key_unhashed, val)\
do\
{\
	dbg_verifyl(hash_table_overflow, (table)->occupied < ht_cap(table)  *ht_max_occupancy, "max occupancy of %.2lf was reached", ht_max_occupancy);\
	u64 key = key_unhashed;\
	if(key == ht_tombstone) key = ht_tombstone_replacement;\
	u64 hash = fnv1a_hash((hashee){.value = key});\
	u64 index = hash & (ht_cap(table) - 1);\
	while((table)->keys[index] != ht_tombstone && (table)->keys[index] != key)\
	{\
		index = (index + 1) & (ht_cap(table) - 1);\
	}\
	if ((table)->keys[index] == ht_tombstone)\
    {\
        (table)->occupied++;\
    }\
	(table)->keys[index] = key;\
	(table)->values[index] = val;\
}while(0)

#define ht_get(table, key_unhashed, outptrptr)\
do\
{\
	u64 key = key_unhashed;\
	if(key == ht_tombstone) key = ht_tombstone_replacement;\
	u64 hash = fnv1a_hash((hashee){.value = key});\
	u64 index = hash & (ht_cap(table) - 1);\
	*(outptrptr) = NULL;\
	while((table)->keys[index] != ht_tombstone)\
	{\
		if((table)->keys[index] == key) \
        { \
            *(outptrptr) = (table)->values + index;\
            break;\
        }\
		index = (index + 1) & (ht_cap(table) - 1);\
	}\
}while(0)

#endif
