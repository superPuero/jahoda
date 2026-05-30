// #ifndef jahoda_nentity
// #define jahoda_nentity

// #define basic_entity_component_func_list(X)\
// 	X(u32)\
// 	X(i32)

// #define nentity_component_id_make_entry(entity_name, t)\
// 	entity_name##_##component_index

// #define nentity_declare(name, component_xlist)\
// typedef enum\
// {\
// 	component_index_end\
// } name##_component_id;\
// static_assert(component_index_end < (sizeof(component_mask) * 8));

// #define component_id_of(c) component_##c

// typedef enum
// {
// 	#define X(c) component_id_of(c) = 1llu << component_##c##_index,
// 	component_list
// 	#undef X
// } component_;


// #endif