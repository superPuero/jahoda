#ifndef jahoda_ent
#define jahoda_ent

#include <jahoda/core/core.h>

// @impl_point
#include "ent_table_list.h"

#define ent_relation_max_children 128
typedef u32 ent_id;
text_declare(ent_name, 32);
sda_declare(ent_id, ent_relation_max_children, ent_id_sda);

typedef struct
{
    ent_id parent;
    ent_id_sda children;
} ent_relation;

#define ent_table_id(type) ent_table_##type
#define ent_table_bit(type) bit(ent_table_##type)

typedef enum
{
    #define X(type) ent_table_id(type),
    ent_table_list
    #undef X
    ent_table_last
} ent_table;

#define ent_table_count ent_table_last
static_assert(ent_table_last < 64);

typedef u64 ent_bitmask;

#define ent_null (ent_id)0
#define ent_null_name (ent_name){0}
#define ent_null_bitmask (ent_bitmask)0
#define ent_null_relation (ent_relation){0}

typedef struct 
{
    u32 capacity;
    
    ent_id id_counter;
    u32 active_count;

    ent_id *active;
    ent_id *kill_list;
    u32 kill_list_size;

    u32 *reverse_active;

    ent_bitmask *bitmasks;
    
    u32 free_list_count;
    ent_id *free_list;

    ent_relation *relations;
    ent_name *names;

    #define X(type)\
    type *type##s;
    ent_table_list
    #undef X
    
    ent_id  *entries[ent_table_count];
    u32     *reverse_entries[ent_table_count];    
    u32     entries_count[ent_table_count];

} ent_manager;


static inline ent_manager *ent_manager_get_current();

static inline ent_manager ent_manager_make(arena *mem, u32 capacity);
static inline void ent_manager_reset(ent_manager *manager);
static inline ent_id ent_manager_spawn(ent_manager *manager);
static inline b8 ent_manager_alive(ent_manager *manager, ent_id id);
static inline void ent_manager_set_name(ent_manager *manager, ent_id id, ent_name name);
static inline void ent_manager_kill(ent_manager *manager, ent_id id);
static inline void ent_manager_kill_recursively(ent_manager *manager, ent_id id);
static inline void ent_manager_kill_schedule(ent_manager *manager, ent_id id);
static inline void ent_manager_kill_exec(ent_manager *manager);
static inline void ent_manager_remove_relation(ent_manager *manager, ent_id parent, ent_id child);
static inline void ent_manager_add_relation(ent_manager *manager, ent_id parent, ent_id child);

#define ent_bitmask_make_1(c) ent_table_bit(c)
#define ent_bitmask_make_2(c1, c2) ent_table_bit(c1) | ent_table_bit(c2)
#define ent_bitmask_make_3(c, ...) ent_bitmask_make_2(__VA_ARGS__) | ent_table_bit(c)
#define ent_bitmask_make_4(c, ...) ent_bitmask_make_3(__VA_ARGS__) | ent_table_bit(c)

#define ent_bitmask_make_router(_1, _2, _3, _4, func, ...) func
#define ent_bitmask_make(...) ((ent_bitmask)ent_bitmask_make_router(__VA_ARGS__, ent_bitmask_make_4, ent_bitmask_make_3, ent_bitmask_make_2, ent_bitmask_make_1)(__VA_ARGS__))

#define ent_has(id, ...) (((_session_ent_manager->bitmasks[id]) & ent_bitmask_make(__VA_ARGS__)) == ent_bitmask_make(__VA_ARGS__))
#define ent_table_entries(type) _session_ent_manager->entries_count[ent_table_id(type)]

#define ent_has_count(id, type) 

#define ent_get(id, type) (_session_ent_manager->type##s + id)
#define ent_try_get(id, type) (ent_get(id, type) * ent_has(id, type))

#define ent_alive(id) (_session_ent_manager->reverse_active[id])

#define ent_get_session() _session_ent_manager

#define ent_add_v(id, type, ...) ({\
    type *_out = NULL;\
    if(ent_alive(id)) {\
        _session_ent_manager->type##s[id] = __VA_ARGS__;\
        if(!ent_has(id, type))\
        {\
            u32 _new_count = _session_ent_manager->entries_count[ent_table_id(type)] + 1;\
            _session_ent_manager->bitmasks[id] |= ent_table_bit(type);\
            _session_ent_manager->entries[ent_table_id(type)][_new_count] = id;\
            _session_ent_manager->reverse_entries[ent_table_id(type)][id] = _new_count;\
            _session_ent_manager->entries_count[ent_table_id(type)] = _new_count;\
        }\
        _out = _session_ent_manager->type##s + id;\
    }\
    _out;\
})

#define ent_add(id, type, ...) ({\
    type *_out = NULL;\
    if(ent_alive(id)) {\
        _session_ent_manager->type##s[id] = (type){__VA_ARGS__};\
        if(!ent_has(id, type))\
        {\
            u32 _new_count = _session_ent_manager->entries_count[ent_table_id(type)] + 1;\
            _session_ent_manager->bitmasks[id] |= ent_table_bit(type);\
            _session_ent_manager->entries[ent_table_id(type)][_new_count] = id;\
            _session_ent_manager->reverse_entries[ent_table_id(type)][id] = _new_count;\
            _session_ent_manager->entries_count[ent_table_id(type)] = _new_count;\
        }\
        _out = _session_ent_manager->type##s + id;\
    }\
    _out;\
})

#define ent_remove(id, type)({\
    if(ent_alive(id) && ent_has(id, type))\
    {\
        u32    _slot = _session_ent_manager->reverse_entries[ent_table_id(type)][id];\
        u32    _last_slot = _session_ent_manager->entries_count[ent_table_id(type)];\
        ent_id _last = _session_ent_manager->entries[ent_table_id(type)][_last_slot];\
        _session_ent_manager->bitmasks[id] &= ~ent_table_bit(type);\
        _session_ent_manager->entries[ent_table_id(type)][_slot] = _last;\
        _session_ent_manager->reverse_entries[ent_table_id(type)][_last] = _slot;\
        _session_ent_manager->reverse_entries[ent_table_id(type)][id] = ent_null;\
        _session_ent_manager->entries_count[ent_table_id(type)]--;\
    }\
}) 

#define ent_parent(id) _session_ent_manager->relations[id].parent
#define ent_children(id) _session_ent_manager->relations[id].children

#define ent_add_relation(parent, child) ent_manager_add_relation(_session_ent_manager, parent, child)
#define ent_remove_relation(parent, child) ent_manager_remove_relation(_session_ent_manager, parent, child)

#define ent_spawn() ent_manager_spawn(_session_ent_manager)

#define ent_saturated() (_session_ent_manager->capacity == _session_ent_manager->id_counter) 

#define ent_with_id() _session_ent

#define ent_spawn_with(...)({\
    ent_id _session_ent = ent_manager_spawn(_session_ent_manager);\
    __VA_ARGS__\
    _session_ent;\
})

#define ent_spawn_bulk_with(count, ...)({\
    for each_index(_bulk_id, count)\
    {\
        ent_spawn_with(__VA_ARGS__);\
    }\
})

#define ent_bulk_id() _bulk_id

#define ent_with(type, ...) ent_add(_session_ent, type, __VA_ARGS__)
#define ent_with_v(type, ...) ent_add_v(_session_ent, type, __VA_ARGS__)

#define ent_with_parent(parent) ent_manager_add_relation(_session_ent_manager, parent, _session_ent)
#define ent_with_children(...) ({\
    ent_relation rel = {.children = {__VA_ARGS__}};\
    for each_index(i, arrsize(rel.children.data))\
    {\
        ent_manager_add_relation(_session_ent_manager, _session_ent, rel.children.data[i]);\
        i++;\
    }\
})

#define ent_name(id) _session_ent_manager->names[id]
#define ent_relations(id) _session_ent_manager->relations[id]

#define ent_with_name(...) ent_name(_session_ent) = ent_name_make(__VA_ARGS__);
#define ent_kill(id) ent_manager_kill(_session_ent_manager, id)
#define ent_kill_recursively(id) ent_manager_kill_recursively(_session_ent_manager, id)

#define ent_kill_schedule(id) ent_manager_kill_schedule(_session_ent_manager, id);
#define ent_kill_exec() ent_manager_kill_exec(_session_ent_manager);

#define ent_each(itname)\
(ent_id *itname = _session_ent_manager->active + 1; itname <= _session_ent_manager->active + _session_ent_manager->active_count; itname++)

#define ent_local_id(id, type) _session_ent_manager->reverse_entries[ent_table_id(type)][id]
#define ent_local_id_0_based(id, type) ({ dbg_verify(id != ent_null, "id = ent_null in ent_local_id_0_based()"); _session_ent_manager->reverse_entries[ent_table_id(type)][id] - 1; })

typedef struct
{
    #define X(type)\
    type *type;\
    type *base_##type;
    ent_table_list
    #undef X
    ent_bitmask mask;
    ent_id *entries;
    ent_name *name;
    ent_name *name_base;
    ent_relation *relation;
    ent_relation *relations_base;
    ent_bitmask *bitmasks;
    u32 count;
    u32 current_entry;
    ent_id id;
} ent_query;

static inline void ent_query_next(ent_query *q);

static inline ent_query ent_query_make(ent_manager *manager, ent_bitmask mask)
{
    ent_query out = {0};

    out.mask = mask;
    out.bitmasks = manager->bitmasks;    
    out.name_base = manager->names;
    out.relations_base = manager->relations;

    #define X(type)\
    out.type = manager->type##s;\
    out.base_##type = manager->type##s;
    ent_table_list
    #undef X

    if (mask == ent_null_bitmask)
    {
        out.entries = manager->active;
        out.count   = manager->active_count;
    }
    else
    {
        i32 best = -1;
        u32 best_count = 0xFFFFFFFFu;

        for each_index(i, ent_table_count)
        {
            if((out.mask & (1 << i)) != (1 << i)) { continue; }

            if (manager->entries_count[i] < best_count)
            {
                best = (i32)i;
                best_count = manager->entries_count[i];
            }
        }

        out.entries = best >= 0 ? manager->entries[best] : NULL;
        out.count   = best >= 0 ? best_count : 0;
    }

    out.current_entry = 1;
    out.id = 0;

    ent_query_next(&out);

    return out;
}

static inline void ent_query_next(ent_query *q)
{
    while (q->current_entry <= q->count)
    {        
        q->id = q->entries[q->current_entry++];
                
        if ((q->bitmasks[q->id] & q->mask) == q->mask)
        {
            #define X(type)\
            q->type = q->base_##type + q->id;    
            ent_table_list
            #undef X
            q->name = q->name_base + q->id;
            q->relation = q->relations_base + q->id;
            return; 
        }
    }

    q->id = 0;
}

static inline b8 ent_query_end(ent_query *q)
{
    return q->id == ent_null;
}

#define ent_each_with(itname, ...)\
(ent_query itname = ent_query_make(_session_ent_manager, ent_bitmask_make(__VA_ARGS__)); !ent_query_end(&itname); ent_query_next(&itname))

#define ent_each_child(itname, of)\
da_each(itname, &_session_ent_manager->relations[of].children)

#define ent_each_of(itname, query)\
(ent_query itname = query; !ent_query_end(&itname); ent_query_next(&itname))

#define ent_each_child_with(itname, of, ...)\
ent_each_child(itname, of) if (!ent_has(*itname, __VA_ARGS__)) {} else

#define ent_session(mgr)\
for(ent_manager *_session_ent_manager = mgr; _session_ent_manager != NULL; _session_ent_manager = NULL)\

static inline ent_manager ent_manager_make(arena *mem, u32 capacity)
{
    ent_manager out = {0};

    out.id_counter = 1;
    
    out.capacity = capacity + 1;
    out.bitmasks = arena_ppush_arr(mem, ent_bitmask, out.capacity);
    out.active = arena_ppush_arr(mem, ent_id, out.capacity);
    out.reverse_active = arena_ppush_arr(mem, u32, out.capacity);
    out.free_list = arena_ppush_arr(mem, ent_id, out.capacity);
    out.kill_list = arena_ppush_arr(mem, ent_id, out.capacity);
    out.names = arena_ppush_arr(mem, ent_name, out.capacity);

    out.relations = arena_ppush_arr(mem, ent_relation, out.capacity);

    #define X(type)\
    out.type##s = arena_ppush_arr(mem, type, out.capacity);
    ent_table_list
    #undef X

    for each_index(i, ent_table_count)
    {
        out.entries[i] = arena_ppush_arr(mem, ent_id, out.capacity);
        out.reverse_entries[i] = arena_ppush_arr(mem, u32, out.capacity);
    }    

    return out;
}

static inline ent_id ent_manager_get_new_id(ent_manager *manager)
{
    ent_id out = 0;

    if(manager->free_list_count)
    {
        out = manager->free_list[--manager->free_list_count];    
        manager->active[manager->active_count + 1] = out;
        manager->reverse_active[out] = manager->active_count + 1;
        manager->active_count++;    
    }
    else if ((manager->active_count + 1) != manager->capacity)
    {                
        out = manager->id_counter++;
        manager->active[manager->active_count + 1] = out;
        manager->reverse_active[out] = manager->active_count + 1;
        manager->active_count++;    
    }

    return out; 
}

static inline b8 ent_manager_alive(ent_manager *manager, ent_id id)
{
    return manager->reverse_active[id];
}

static inline void ent_manager_kill_schedule(ent_manager *manager, ent_id id)
{
    manager->kill_list[manager->kill_list_size++] = id;
}

static inline void ent_manager_kill_exec(ent_manager *manager)
{
    for each_index(i, manager->kill_list_size)
    {
        ent_manager_kill(manager, manager->kill_list[i]);
    }
}

static inline void ent_manager_remove_relation(ent_manager *manager, ent_id parent, ent_id child)
{
    if(!ent_manager_alive(manager, parent) || !ent_manager_alive(manager, child))
    {
        return;        
    }

    da_if_find(&manager->relations[parent].children, child, index)
    {
        da_swap_and_pop(&manager->relations[parent].children, index);
        manager->relations[child].parent = ent_null;
    }       
}

static inline void ent_manager_add_relation(ent_manager *manager, ent_id parent, ent_id child)
{
    if((manager->reverse_active[parent] == ent_null) || (manager->reverse_active[child] == ent_null))
    {
        return;        
    }

    da_if_find(&manager->relations[parent].children, child, index)
    {
        return;
    }       

    sda_append(&manager->relations[parent].children, child);
    manager->relations[child].parent = parent;    
}

static inline ent_id ent_manager_spawn(ent_manager *manager)
{
    ent_id id = ent_manager_get_new_id(manager);

    manager->relations[id] = ent_null_relation;    
    manager->bitmasks[id] = ent_null_bitmask;
    manager->names[id] = ent_null_name;
    
    return id;
}

static inline void ent_manager_kill(ent_manager *manager, ent_id id)
{
    if(manager->reverse_active[id] == ent_null)
    {
        return;
    }
    
    for da_each(it, &manager->relations[id].children)
    {
        ent_manager_remove_relation(manager, id, *it);
    }
    
    ent_session(manager)
    {
        #define X(type)\
        if(ent_has(id, type))\
        {\
            ent_remove(id, type);\
        }            
        ent_table_list
        #undef X
    }    

    manager->free_list[manager->free_list_count++] = id;
    manager->active[manager->reverse_active[id]] = manager->active[manager->active_count];
    manager->reverse_active[manager->active[manager->reverse_active[id]]] = id;
    manager->reverse_active[id] = ent_null;
    manager->active_count--;
}

static inline void ent_manager_reset(ent_manager *manager)
{
    manager->id_counter = 1;
    
    manager->kill_list_size = 0;
    manager->free_list_count = 0;
    manager->active_count = 0;

    memset(manager->reverse_active, 0, sizeof(u32) * manager->capacity);

    for each_index(i, ent_table_count)
    {
        manager->entries_count[i] = 0;
        memset(manager->reverse_entries[i], 0, sizeof(u32) * manager->capacity);
    }
}

static inline void ent_manager_kill_recursively(ent_manager *manager, ent_id id)
{
    if(manager->reverse_active[id] == ent_null)
    {
        return;
    }
    
    for da_each(it, &manager->relations[id].children)
    {
        ent_manager_kill_recursively(manager, *it);        
        ent_manager_kill(manager, *it);
    }

    ent_manager_kill(manager, id);
}

#endif