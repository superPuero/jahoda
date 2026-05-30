#ifndef jahoda_ht
#define jahoda_ht

#include "da.h"
#include "hash.h"

#define ht_tombstone 0
#define ht_tombstone_replacement 0xdeadbeef

#define ht_max_occupancy 0.75

da_declare(u64, ht_key_da);

#define sht_declare(data_type, size_power_of_two, ht_name)\
typedef struct\
{\
	data_type values[(1 << size_power_of_two)];\
	data_type *value_it;\
	u64 keys[(1 << size_power_of_two)];\
	u64 *key_it;\
	u64 occupied;\
} ht_name;

#define sht_cap(table) arrsize((table)->values)

#define sht_insert(table, key_unhashed, val)\
do\
{\
	dbg_verifyl(hash_table_overflow, (table)->occupied < sht_cap(table)  *ht_max_occupancy, "max occupancy of %.2lf was reached", ht_max_occupancy);\
	u64 key = key_unhashed;\
	if(key == ht_tombstone) key = ht_tombstone_replacement;\
	u64 hash = fnv1a_hash((hashee){.value = key});\
	u64 index = hash & (sht_cap(table) - 1);\
	while((table)->keys[index] != ht_tombstone && (table)->keys[index] != key)\
	{\
		index = (index + 1) & (sht_cap(table) - 1);\
	}\
	if ((table)->keys[index] == ht_tombstone)\
    {\
        (table)->occupied++;\
    }\
	(table)->keys[index] = key;\
	(table)->values[index] = val;\
}while(0)

#define sht_get(table, key_unhashed, outptrptr)\
do\
{\
	u64 key = key_unhashed;\
	if(key == ht_tombstone) key = ht_tombstone_replacement;\
	u64 hash = fnv1a_hash((hashee){.value = key});\
	u64 index = hash & (sht_cap(table) - 1);\
	*(outptrptr) = NULL;\
	while((table)->keys[index] != ht_tombstone)\
	{\
		if((table)->keys[index] == key) \
        { \
            *(outptrptr) = (table)->values + index;\
            break;\
        }\
		index = (index + 1) & (sht_cap(table) - 1);\
	}\
}while(0)

#define sht_foreach(ht)\
for(uz _i = ((ht)->value_it = NULL, (ht)->key_it = NULL, 0); _i < sht_cap(ht) && ((ht)->value_it = (ht)->values + _i, (ht)->key_it = (ht)->keys + _i, 1); ++_i)

#endif
