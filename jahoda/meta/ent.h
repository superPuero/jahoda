#include <jahoda/core/core.h>

#ifndef ent_base
#define ent_base

typedef struct {} ent_root;
typedef u64 ent_bitmask;

#define ent_get_session() _ent_session

#define ent_spawn(manager) ({\
    u32 out = 0;\
    if((manager)->free_list_count)\
    {\
        out = (manager)->free_list[--(manager)->free_list_count];    \
        (manager)->active[(manager)->active_count + 1] = out;\
        (manager)->reverse_active[out] = (manager)->active_count + 1;\
        (manager)->active_count++;    \
    }\
    else if (((manager)->active_count + 1) != (manager)->capacity)\
    {                \
        out = (manager)->id_counter++;\
        (manager)->active[(manager)->active_count + 1] = out;\
        (manager)->reverse_active[out] = (manager)->active_count + 1;\
        (manager)->active_count++;    \
    }\
    zero_fill(&(manager)->bitmasks[out]);\
    \
    if((manager)->relations)\
    {\
        zero_fill(&(manager)->relations[out]);\
        ent_add((manager), out, ent_root, {});\
    }\
    if((manager)->names)\
    {\
        (manager)->names[out] = (text32){0};\
    }\
    out;\
})

#define ent_force_type_lint(type) ((void)(type){})

#define ent_table_id(manager, type) (manager)->type##_id
#define ent_table_bit(manager, type) (manager)->type##_bit

#define ent_bitmask_make_1(manager, c) ent_table_bit(manager, c)
#define ent_bitmask_make_2(manager, c1, c2) ent_table_bit(manager, c1) | ent_table_bit(manager, c2)
#define ent_bitmask_make_3(manager, c, ...) ent_bitmask_make_2(manager, __VA_ARGS__) | ent_table_bit(manager, c)
#define ent_bitmask_make_4(manager, c, ...) ent_bitmask_make_3(manager, __VA_ARGS__) | ent_table_bit(manager, c)
#define ent_bitmask_make_5(manager, c, ...) ent_bitmask_make_4(manager, __VA_ARGS__) | ent_table_bit(manager, c)
#define ent_bitmask_make_6(manager, c, ...) ent_bitmask_make_5(manager, __VA_ARGS__) | ent_table_bit(manager, c)
#define ent_bitmask_make_7(manager, c, ...) ent_bitmask_make_6(manager, __VA_ARGS__) | ent_table_bit(manager, c)
#define ent_bitmask_make_8(manager, c, ...) ent_bitmask_make_7(manager, __VA_ARGS__) | ent_table_bit(manager, c)

#define ent_bitmask_make_router(manager, _1, _2, _3, _4, _5, _6, _7, _8, func, ...) func
#define ent_bitmask_make(manager, ...) ((ent_bitmask)ent_bitmask_make_router(manager, __VA_ARGS__, ent_bitmask_make_8, ent_bitmask_make_7, ent_bitmask_make_6, ent_bitmask_make_5, ent_bitmask_make_4, ent_bitmask_make_3, ent_bitmask_make_2, ent_bitmask_make_1)(manager, __VA_ARGS__))

#define ent_active(manager, id) (manager)->reverse_active[id]

#define ent_table_entries(manager, type) (manager)->entries_count[ent_table_id(manager, type)]

#define ent_has_from_id(manager, id, table_id) (((manager)->bitmasks[id] & bit(table_id)) == bit(table_id))
#define ent_has(manager, id, type) (ent_force_type_lint(type), (ent_has_from_id(manager, id, ent_table_id(manager, type))))

#define ent_get(manager, id, type) ((type*)((manager)->tables[(manager)->type##_id]) + id)

#define ent_add(manager, id, type, ...)({\
    type *_out = NULL;\
    u32 table_id = ent_table_id(manager, type);\
    if(ent_active(manager, id)) {\
        _out = (type*)((manager)->tables[table_id]) + id;\
        *_out = (type)__VA_ARGS__;\
        if(!ent_has(manager, id, type))\
        {\
            u32 _new_count = (manager)->entries_count[table_id] + 1;\
            (manager)->bitmasks[id] |= ent_table_bit(manager, type);\
            (manager)->entries[table_id][_new_count] = id;\
            (manager)->reverse_entries[table_id][id] = _new_count;\
            (manager)->entries_count[table_id] = _new_count;\
        }\
    }\
    _out;\
})

#define ent_name(manager, id) (manager)->names[id]
#define ent_name_set(manager, id, ...) (manager)->names[id] = text32_make(__VA_ARGS__)

#define ent_remove_from_id(manager, id, table_id)({\
    if(ent_active(manager, id) && ent_has_from_id(manager, id, table_id))\
    {\
        u32    _slot = (manager)->reverse_entries[table_id][id];\
        u32    _last_slot = (manager)->entries_count[table_id];\
        u32 _last = (manager)->entries[table_id][_last_slot];\
        (manager)->bitmasks[id] &= ~bit(table_id);\
        (manager)->entries[table_id][_slot] = _last;\
        (manager)->reverse_entries[table_id][_last] = _slot;\
        (manager)->reverse_entries[table_id][id] = 0;\
        (manager)->entries_count[table_id]--;\
    }\
}) 

#define ent_remove(manager, id, type)({\
    ent_force_type_lint(type);\
    ent_remove_from_id(manager, id, ent_table_id(manager, type));\
}) 

#define ent_add_relation(manager, parent_, child)\
do{\
    if(ent_active(manager, parent_) && ent_active(manager, child))\
    {\
        u32 index = da_find(&(manager)->relations[parent_].children, child);\
        if(index == (manager)->relations[parent_].children.occupied)\
        {\
            sda_append(&(manager)->relations[parent_].children, child);\
            (manager)->relations[child].parent = parent_;\
            ent_remove(manager, child, ent_root);\
        }\
    }\
}while(0)\

#define ent_remove_relation(manager, parent_, child)({\
    if(ent_active(manager, parent_) && ent_active(manager, child))\
    {\
        da_if_find(&(manager)->relations[parent_].children, child, index)\
        {\
            da_swap_and_pop(&(manager)->relations[parent_].children, index);\
            (manager)->relations[child].parent = 0;\
            ent_add(manager, child, ent_root, {});\
        }\
    }\
})\

#define ent_despawn(manager, id)({\
    if(ent_active(manager, id))\
    {\
        for da_each(it, &(manager)->relations[id].children)\
        {\
            ent_remove_relation(manager, id, *it);\
        }\
        for each_index(i, (manager)->table_count)\
        {\
            ent_remove_from_id(manager, id, i);\
        }\
        (manager)->free_list[(manager)->free_list_count++] = id;\
        (manager)->active[(manager)->reverse_active[id]] = (manager)->active[(manager)->active_count];\
        (manager)->reverse_active[(manager)->active[(manager)->reverse_active[id]]] = id;\
        (manager)->reverse_active[id] = 0;\
        (manager)->active_count--;\
    }\
})

typedef struct
{
    arena *mem;
    u32 capacity;
    text128 name;
    b8 names;
    b8 relations;
} ent_manager_config;

#define ent_manager_make(spec, ...) concat(spec, _ent_manager_make)((ent_manager_config){__VA_ARGS__})

#define ent_manager_reset(manager)({\
    (manager)->id_counter = 1;\
    (manager)->kill_list_size = 0;\
    (manager)->free_list_count = 0;\
    (manager)->active_count = 0;\
    memset((manager)->reverse_active, 0, sizeof(u32) * (manager)->capacity);\
    for each_index(i, (manager)->table_count)\
    {\
        (manager)->entries_count[i] = 0;\
        memset((manager)->reverse_entries[i], 0, sizeof(u32) * (manager)->capacity);\
    }\
})

typedef struct
{
    ent_bitmask include_mask;
    ent_bitmask exclude_mask;
} ent_query_config;

#define ent_query_make(manager, ...)({\
    ent_query_config cfg = {__VA_ARGS__};\
    typeof((manager)->_query) out = {0};\
    out.include_mask = cfg.include_mask;\
    out.exclude_mask = cfg.exclude_mask;\
    out.bitmasks = (manager)->bitmasks;\
    if (out.include_mask == 0)\
    {\
        out.entries = (manager)->active + 1; /* skip ent_null */ \
        out.entries_count   = (manager)->active_count;\
    }\
    else\
    {\
        u32 best = -1;\
        u32 best_count = ~0;\
        for each_index(i, (manager)->table_count)\
        {\
            if(!has_bit(out.include_mask, i)) { continue; }\
            if ((manager)->entries_count[i] < best_count)\
            {\
                best = (i32)i;\
                best_count = (manager)->entries_count[i];\
            }\
        }\
        out.entries = (manager)->entries[best] + 1;  /* skip ent_null */ \
        out.entries_count   = best_count;\
        out.current_entry = 0;\
    }\
    out;\
})

#define ent_query_split(query, ways, mem)({\
    typeof(*query) *out = arena_ppush_arr(mem, typeof(*query), ways);\
    u32 chunk   = ((query)->entries_count + ways - 1) / (ways);  \
    for each_index(i, ways)\
    {\
        out[i] = *(query);\
        u32 start = i * chunk;\
        if (start >= (query)->entries_count) \
        {\
            out[i].entries_count = 0;\
            continue;\
        }\
        u32 rem              = (query)->entries_count - start;\
        out[i].entries       = (query)->entries + start;\
        out[i].entries_count = (rem < chunk) ? rem : chunk;\
    }\
    out;\
})

#define ent_query_next(query)({\
    b8 out = false;\
    while ((query)->current_entry < (query)->entries_count)\
    {        \
        (query)->id = (query)->entries[(query)->current_entry++];\
        if (has_bitset((query)->bitmasks[(query)->id], (query)->include_mask) && !has_bitset_any((query)->bitmasks[(query)->id], (query)->exclude_mask)) \
        {\
            out = true;\
            break;\
        }\
    }\
    out;\
})

#define ent_local_id_1_based(manager, id, type) (manager)->reverse_entries[ent_table_id(manager, type)][id]
#define ent_local_id(manager, id, type) ({ dbg_verify(id != 0, "id = 0 in ent_local_id_0_based()"); (manager)->reverse_entries[ent_table_id(manager, type)][id] - 1; })

#define ent_each_child(manager, itname, of)\
da_each(itname, &(manager)->relations[of].children)

#define ent_table_name(manager, type) (manager)->table_names[(manager)->type##_id]
#define ent_table_name_from_id(manager, table_id) (manager)->table_names[table_id]

#endif

#define ent_all_tables\
    X(ent_root)\
    ent_tables

#define ent_spec_decl(x) concat(ent_spec, _##x)

u32 ent_spec_decl(foo);

typedef u32 ent_spec_decl(ent_id);
sda_declare(ent_spec_decl(ent_id), ent_max_children, ent_spec_decl(ent_id_sda));

typedef struct
{
    ent_spec_decl(ent_id) parent;
    ent_spec_decl(ent_id_sda) children;
} ent_spec_decl(ent_relation);


enum
{
    #define X(type) ent_spec_decl(ent_##type),
    ent_all_tables
    #undef X
    ent_spec_decl(ent_table_count)
};

static_assert(ent_spec_decl(ent_table_count) < 64);

typedef struct
{
    ent_bitmask include_mask;
    ent_bitmask exclude_mask;
    u32 entries_count;
    ent_spec_decl(ent_id) *entries;
    ent_bitmask *bitmasks;
    u32 current_entry;
    ent_spec_decl(ent_id) id;
} ent_spec_decl(ent_query);

typedef struct 
{
    text128 name;
    u32 capacity;
    ent_spec_decl(ent_id) id_counter;
    u32 active_count;
    ent_spec_decl(ent_id) *active;
    u32 *reverse_active;
    ent_bitmask *bitmasks;
    ent_spec_decl(ent_relation) *relations;
    text32 *names;
    u32 kill_list_size;
    ent_spec_decl(ent_id) *kill_list;
    u32 free_list_count;
    ent_spec_decl(ent_id) *free_list;
    ent_spec_decl(ent_id)  *entries[ent_spec_decl(ent_table_count)];
    u32     table_count;
    const char *table_names[ent_spec_decl(ent_table_count)];
    uz table_sizes[ent_spec_decl(ent_table_count)];
    void    *tables[ent_spec_decl(ent_table_count)];\
    u32     *reverse_entries[ent_spec_decl(ent_table_count)];
    u32     entries_count[ent_spec_decl(ent_table_count)];
    #define X(type)\
    u32     type##_id;\
    u32     type##_bit;
    ent_all_tables
    #undef X
    ent_spec_decl(ent_query) _query;
} ent_spec_decl(ent_manager);

static inline ent_spec_decl(ent_manager) ent_spec_decl(ent_manager_make)(ent_manager_config cfg)
{
    ent_spec_decl(ent_manager) out = {0};
    out.name = cfg.name;
    out.id_counter = 1;
    out.capacity = cfg.capacity + 1;
    out.bitmasks = arena_ppush_arr(cfg.mem, ent_bitmask, out.capacity);
    out.active = arena_ppush_arr(cfg.mem, ent_spec_decl(ent_id), out.capacity);
    out.reverse_active = arena_ppush_arr(cfg.mem, u32, out.capacity);
    out.free_list = arena_ppush_arr(cfg.mem, ent_spec_decl(ent_id), out.capacity);
    out.kill_list = arena_ppush_arr(cfg.mem, ent_spec_decl(ent_id), out.capacity);

    if(cfg.names)
    {
        out.names = arena_ppush_arr(cfg.mem, text32, out.capacity);
    }

    if(cfg.relations)
    {
        out.relations = arena_ppush_arr(cfg.mem, ent_spec_decl(ent_relation), out.capacity);
    }

    out.table_count = ent_spec_decl(ent_table_count);

    #define X(type)\
    out.type##_id = ent_spec_decl(ent_##type);\
    out.type##_bit = bit(ent_spec_decl(ent_##type));\
    out.tables[out.type##_id] = arena_ppush_arr(cfg.mem, type, out.capacity);\
    out.entries[out.type##_id] = arena_ppush_arr(cfg.mem, ent_spec_decl(ent_id), out.capacity);\
    out.reverse_entries[ out.type##_id] = arena_ppush_arr(cfg.mem, u32, out.capacity);\
    out.entries_count[out.type##_id] = 1;\
    out.table_sizes[out.type##_id] = sizeof(type);\
    out.table_names[out.type##_id] = #type;
    ent_all_tables
    #undef X     

    return out;
}

// static inline void ent_manager_kill_recursively(ent_manager *manager, ent_id id)
// {
//     if(manager->reverse_active[id] == ent_null)
//     {
//         return;
//     }
    
//     for da_each(it, &manager->relations[id].children)
//     {
//         ent_manager_kill_recursively(manager, *it);        
//         ent_manager_kill(manager, *it);
//     }

//     ent_manager_kill(manager, id);
// }

#undef ent_all_tables

#undef ent_spec_decl

#undef ent_spec
#undef ent_max_children
#undef ent_tables

// #define ent_has(id, ...) (((ent_get_session()->bitmasks[id]) & ent_bitmask_make(__VA_ARGS__)) == ent_bitmask_make(__VA_ARGS__))
// #define ent_has_table_id(id, table) (((ent_get_session()->bitmasks[id]) & bit(table)) == bit(table))

// #define ent_table_entries(type) ent_get_session()->entries_count[ent_table_id(type)]

// #define ent_has_count(id, type) 

// #define ent_get(id, type) ((type*)ent_get_session()->tables[ent_table_id(type)] + id)
// #define ent_try_get(id, type) (ent_has(id, type) ? ent_get(id, type) : NULL)

// #define ent_alive(id) (ent_get_session()->reverse_active[id])

// #define ent_add_v(id, type, ...) ({\
//     type *_out = NULL;\
//     if(ent_alive(id)) {\
//     type* table = (type*)(ent_get_session()->tables[ent_table_id(type)]);\
//        table[id] = __VA_ARGS__;\
//         if(!ent_has(id, type))\
//         {\
//             u32 _new_count = ent_get_session()->entries_count[ent_table_id(type)] + 1;\
//             ent_get_session()->bitmasks[id] |= ent_table_bit(type);\
//             ent_get_session()->entries[ent_table_id(type)][_new_count] = id;\
//             ent_get_session()->reverse_entries[ent_table_id(type)][id] = _new_count;\
//             ent_get_session()->entries_count[ent_table_id(type)] = _new_count;\
//         }\
//         _out = table + id;\
//     }\
//     _out;\
// })

// #define ent_add(id, type, ...) ({\
//     type *_out = NULL;\
//     if(ent_alive(id)) {\
//         type* table = (type*)(ent_get_session()->tables[ent_table_id(type)]);\
//         table[id] = (type){__VA_ARGS__};\
//         if(!ent_has(id, type))\
//         {\
//             u32 _new_count = ent_get_session()->entries_count[ent_table_id(type)] + 1;\
//             ent_get_session()->bitmasks[id] |= ent_table_bit(type);\
//             ent_get_session()->entries[ent_table_id(type)][_new_count] = id;\
//             ent_get_session()->reverse_entries[ent_table_id(type)][id] = _new_count;\
//             ent_get_session()->entries_count[ent_table_id(type)] = _new_count;\
//         }\
//         _out = table + id;\
//     }\
//     _out;\
// })

// #define ent_parent(id) ent_get_session()->relations[id].parent
// #define ent_children(id) ent_get_session()->relations[id].children

// #define ent_add_relation(parent, child) ent_manager_add_relation(ent_get_session(), parent, child)
// #define ent_remove_relation(parent, child) ent_manager_remove_relation(ent_get_session(), parent, child)

// #define ent_spawn() ent_manager_spawn(ent_get_session())

// #define ent_saturated() (ent_get_session()->capacity == ent_get_session()->id_counter) 

// #define ent_with_id() _session_ent

// #define ent_spawn_with(...)({\
//     ent_id _session_ent = ent_manager_spawn(ent_get_session());\
//     __VA_ARGS__\
//     _session_ent;\
// })

// #define ent_spawn_bulk_with(count, ...)({\
//     for each_index(_bulk_id, count)\
//     {\
//         ent_spawn_with(__VA_ARGS__);\
//     }\
// })

// #define ent_bulk_id() _bulk_id

// #define ent_with(type, ...) ent_add(_session_ent, type, __VA_ARGS__)
// #define ent_with_v(type, ...) ent_add_v(_session_ent, type, __VA_ARGS__)

// #define ent_with_parent(parent) ent_manager_add_relation(ent_get_session(), parent, _session_ent)
// #define ent_with_children(...) ({\
//     ent_id children[] = {__VA_ARGS__};\
//     for each_index(i, arrlen(children))\
//     {\
//         ent_manager_add_relation(ent_get_session(), _session_ent, children[i]);\
//     }\
// })

// #define ent_name_of(id) ent_get_session()->names[id]
// #define ent_relations(id) ent_get_session()->relations[id]

// #define ent_with_name(...) ent_name_of(_session_ent) = ent_name_make(__VA_ARGS__);
// #define ent_kill(id) ent_manager_kill(ent_get_session(), id)
// #define ent_kill_recursively(id) ent_manager_kill_recursively(ent_get_session(), id)

// #define ent_kill_schedule(id) ent_manager_kill_schedule(ent_get_session(), id);
// #define ent_kill_exec() ent_manager_kill_exec(ent_get_session());

// #define ent_each(itname)\
// (ent_id *itname = ent_get_session()->active + 1; itname <= ent_get_session()->active + ent_get_session()->active_count; itname++)

// #define ent_local_id_1_based(id, type) ent_get_session()->reverse_entries[ent_table_id(type)][id]
// #define ent_local_id(id, type) ({ dbg_verify(id != ent_null, "id = ent_null in ent_local_id_0_based()"); ent_get_session()->reverse_entries[ent_table_id(type)][id] - 1; })

// typedef struct
// {
//     ent_bitmask include_mask;
//     ent_bitmask exclude_mask;
//     u32 entries_count;
//     ent_id *entries;
//     ent_bitmask *bitmasks;
//     u32 current_entry;
//     ent_id id;
// } ent_query;

// static inline b8 ent_query_next(ent_query *q)
// {
//     while (q->current_entry < q->entries_count)
//     {        
//         q->id = q->entries[q->current_entry++];           
//         if (has_bitset(q->bitmasks[q->id], q->include_mask) && !has_bitset_any(q->bitmasks[q->id], q->exclude_mask)) 
//         {
//             return true; 
//         }
//     }

//     return false;
// }

// typedef struct
// {
//     ent_bitmask include_mask;
//     ent_bitmask exclude_mask;
//     ent_manager *manager;
// } ent_query_params;

// static inline ent_query ent_manager_query_make(ent_query_params params)
// {
//     ent_query out = {0};

//     out.include_mask = params.include_mask;
//     out.exclude_mask = params.exclude_mask;

//     out.bitmasks = params.manager->bitmasks;    

//     if (params.include_mask == ent_null_bitmask)
//     {
//         out.entries = params.manager->active + 1; // skip ent_null
//         out.entries_count   = params.manager->active_count;
//     }
//     else
//     {
//         u32 best = -1;
//         u32 best_count = ~0;

//         for each_index(i, ent_table_count)
//         {
//             if(!has_bit(out.include_mask, i)) { continue; }

//             if (params.manager->entries_count[i] < best_count)
//             {
//                 best = (i32)i;
//                 best_count = params.manager->entries_count[i];
//             }
//         }

//         out.entries = params.manager->entries[best] + 1; // skip ent_null
//         out.entries_count   = best_count;
//         out.current_entry = 0;
//     }

//     return out;
// }

// #define ent_query_make(...) ent_manager_query_make((ent_query_params){.manager = ent_get_session(), __VA_ARGS__})

// static inline ent_query *ent_query_split(ent_query *query, u32 ways, arena* mem)
// {
//     ent_query *out = arena_ppush_arr(mem, ent_query, ways);

//     u32 chunk   = (query->entries_count + ways - 1) / ways;  

//     for each_index(i, ways)
//     {
//         out[i] = *query;
//         u32 start = i * chunk;

//         if (start >= query->entries_count) 
//         {
//             out[i].entries_count = 0;
//             continue;
//         }

//         u32 rem              = query->entries_count - start;
//         out[i].entries       = query->entries + start;
//         out[i].entries_count = (rem < chunk) ? rem : chunk;      
//     }

//     return out;
// }

// #define ent_each_child(itname, of)\
// da_each(itname, &ent_get_session()->relations[of].children)

// #define ent_each_child_with(itname, of, ...)\
// ent_each_child(itname, of) if (!ent_has(*itname, __VA_ARGS__)) {} else

// static inline ent_manager ent_manager_make_(ent_manager_config cfg)
// {
//     ent_manager out = {0};
//     out.name = cfg.name;
//     out.allocation_mask = cfg.allocation_mask;

//     out.id_counter = 1;
//     out.capacity = cfg.capacity + 1;
//     out.bitmasks = arena_ppush_arr(cfg.mem, ent_bitmask, out.capacity);
//     out.active = arena_ppush_arr(cfg.mem, ent_id, out.capacity);
//     out.reverse_active = arena_ppush_arr(cfg.mem, u32, out.capacity);
//     out.free_list = arena_ppush_arr(cfg.mem, ent_id, out.capacity);
//     out.kill_list = arena_ppush_arr(cfg.mem, ent_id, out.capacity);

//     if(cfg.names)
//     {
//         out.names = arena_ppush_arr(cfg.mem, ent_name, out.capacity);
//     }

//     if(cfg.relations)
//     {
//         out.relations = arena_ppush_arr(cfg.mem, ent_relation, out.capacity);
//         out.allocation_mask |= ent_table_bit(ent_root);
//     }

//     for each_index(i, ent_table_count)
//     {
//         if(has_bit(out.allocation_mask, i))
//         {
//             out.tables[i] = arena_push_bytes(
//                 cfg.mem, 
//                 ent_table_size[i] * out.capacity, 
//                 ent_table_alignment[i], 
//                 true
//             );

//             out.entries[i] = arena_ppush_arr(cfg.mem, ent_id, out.capacity);
//             out.reverse_entries[i] = arena_ppush_arr(cfg.mem, u32, out.capacity);
//         }
//     }    

//     return out;
// }

// static inline ent_id ent_manager_get_new_id(ent_manager *manager)
// {
//     ent_id out = 0;

//     if(manager->free_list_count)
//     {
//         out = manager->free_list[--manager->free_list_count];    
//         manager->active[manager->active_count + 1] = out;
//         manager->reverse_active[out] = manager->active_count + 1;
//         manager->active_count++;    
//     }
//     else if ((manager->active_count + 1) != manager->capacity)
//     {                
//         out = manager->id_counter++;
//         manager->active[manager->active_count + 1] = out;
//         manager->reverse_active[out] = manager->active_count + 1;
//         manager->active_count++;    
//     }

//     return out; 
// }

// static inline b8 ent_manager_alive(ent_manager *manager, ent_id id)
// {
//     return manager->reverse_active[id];
// }

// static inline void ent_manager_kill_schedule(ent_manager *manager, ent_id id)
// {
//     manager->kill_list[manager->kill_list_size++] = id;
// }

// static inline void ent_manager_kill_exec(ent_manager *manager)
// {
//     for each_index(i, manager->kill_list_size)
//     {
//         ent_manager_kill(manager, manager->kill_list[i]);
//     }
// }


