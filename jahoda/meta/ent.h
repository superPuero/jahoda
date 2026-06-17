#ifndef jahoda_entity
#define jahoda_entity

#include <jahoda/core/core.h>
#include <jahoda/core/math.h>

typedef vec2_f32 position;
typedef vec2_f32 velocity;

#define component_list\
	X(position)\
	X(velocity)
	
typedef u64 component_mask;

#define component_bitid_of(c) component_##c##_index

typedef enum {
    #define X(c) component_bitid_of(c),
    component_list
    #undef X
	component_index_end
} component_id;

static_assert(component_index_end < (sizeof(component_mask) * 8));

#define component_id_of(c) component_##c

typedef enum
{
	#define X(c) component_id_of(c) = 1llu << component_##c##_index,
	component_list
	#undef X
} component_;

#define entity_has(entity, c) ((entity)->components & component_##c)
#define entity_get(entity, c) ((entity)->c)

#define entity_remove(entity, c) ((entity)->components &= ~component_##c)

#define entity_add(entity, c, ...)\
do{\
	if(entity_has(entity, c))\
	{ / *@todo: this is not it*/ \
		err("adding" #c "to entity that already has it");\
	}\
	(entity)->c = ( c )__VA_ARGS__;\
	(entity)->components |= component_##c;\
}while(0)

typedef struct entity_manager_s entity_manager;

typedef struct
{	
	#define X(c) c c;
	component_list
	#undef X
	entity_manager *manager;
	component_mask components;
	bool8 active;
} entity;

da_declare(entity, entity_da);

typedef component_mask entity_filter; 

typedef struct
{
	component_mask entries[64];
} entity_filter_params; 

#define entity_filter_make(...) entity_filter_make_((entity_filter_params){.entries = {__VA_ARGS__}})

entity_filter entity_filter_make_(entity_filter_params params);

#define entity_filter_pass(entity, key) (((entity)->components & key) == key && (entity)->active)

da_declare(uz, entity_free_list);

struct entity_manager_s
{	
	arena *memory;
	entity_da entities;
	entity_free_list free_list;
};

typedef struct
{
	uz capacity;
	arena *memory;
} entity_manager_params;

#define entity_manager_make(...) entity_manager_make_((entity_manager_params){__VA_ARGS__}); 

entity_manager entity_manager_make_(entity_manager_params params);
entity *entity_manager_make_entity(entity_manager *manager);

void entity_manager_release(entity_manager *manager);

#endif