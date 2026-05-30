#include "ent.h"

entity_filter entity_filter_make_(entity_filter_params params)
{
	entity_filter key = {0};

	uz current = 0;
	while(params.entries[current] != 0)
	{
		key |= params.entries[current];
		current++;
	}	

	return key;
}	

entity_manager entity_manager_make_(entity_manager_params params)
{
	entity_manager out = {
		.memory = params.memory,
		.entities = {0}
	};

	da_reserve(out.memory, &out.entities, params.capacity);

	return out;
}

void entity_manager_release(entity_manager *manager)
{
	//@todo: this function is not needed, resource management in this structure is purely external (it does not allocate nor release memory on its own)
}

entity *entity_manager_make_entity(entity_manager *manager)
{
	entity *out = NULL;
	if(manager->free_list.occupied)
	{
		entity *out = manager->entities.data + *da_last(&manager->free_list);
		out->active = true;
		out->components = 0;
		manager->free_list.occupied--;
	}
	else
	{
		dbg_verify(
			manager->entities.capacity >= manager->entities.occupied, 
			"entity manager overflow, max ammount of entitities reached %uz", 
			manager->entities.occupied
		);

		da_append(manager->memory, &manager->entities, (entity){0});
		out = da_last(&manager->entities);
		out->manager = manager;	
		out->active = true;
	}

	return out;
}
