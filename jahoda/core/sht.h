#ifndef jahoda_sht
#define jahoda_sht

#include "da.h"
#include "str.h"
#include "hash.h"

#define sht_tombstone 0
#define sht_max_occupancy 0.75

#define sht_declare(data_type, max_string_size, size_power_of_two, ht_name)\
typedef struct\
{\
	data_type values[(1 << size_power_of_two)];\
	u8 keys[(1 << size_power_of_two)][max_string_size + 2];\
	u64 occupied;\
} ht_name;

#define sht_keysize(table) sizeof((table)->keys[0]) - 2
#define sht_sentinel(table) sizeof((table)->keys[0]) - 1

#define sht_cap(table) arrsize((table)->values)

#define sht_insert(table, key_strv, ...)\
do\
{\
    strv _key = (key_strv);\
    dbg_verifyl(string_hash_table, sht_keysize(table) >= _key.len, "(%.*s) is too big, max string key size is %u", strv_fmt(&_key), sizeof((table)->keys[0]) - 1);\
	dbg_verifyl(hash_table_overflow, (table)->occupied < sht_cap(table)  *ht_max_occupancy, "max occupancy of %.2lf was reached", ht_max_occupancy);\
	u64 hash = fnv1a_hash_strv(_key);\
	u64 index = hash & (sht_cap(table) - 1);\
    	while((table)->keys[index][sht_sentinel(table)] != '\0' && (strncmp(_key.data, (table)->keys[index], sht_keysize(table)) != 0))\
	{\
		index = (index + 1) & (sht_cap(table) - 1);\
	}\
	if((table)->keys[index][sht_sentinel(table)] != '\0')\
    {\
        (table)->occupied++;\
    }\
    memcpy((table)->keys[index], _key.data, sht_keysize(table));\
    (table)->keys[index][_key.len] = '\0';\
    (table)->keys[index][sht_sentinel(table)] = 'x';\
	(table)->values[index] = __VA_ARGS__;\
}while(0)

#define sht_remove(table, key_strv)\
do\
{\
	strv _key = (key_strv);\
	u64 hash = fnv1a_hash_strv(_key);\
	u64 index = hash & (sht_cap(table) - 1);\
    (table)->keys[index][sht_sentinel(table)] = '\0';\
}while(0)

#define sht_get(table, key_strv, outptrptr)\
do\
{\
	strv _key = (key_strv);\
	u64 hash = fnv1a_hash_strv(_key);\
	u64 index = hash & (sht_cap(table) - 1);\
	outptrptr = NULL;\
	while((table)->keys[index][sht_sentinel(table)] != '\0')\
	{\
		if(strncmp(_key.data, (table)->keys[index], sht_keysize(table)) == 0)\
        {\            
            outptrptr = (table)->values + index;\
            break;\
        }\
		index = (index + 1) & (sht_cap(table) - 1);\
	}\
}while(0)

#define sht_each(key_it, val_it, table)\
()  

#endif
