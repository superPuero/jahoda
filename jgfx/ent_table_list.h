#ifndef jahoda_ent_table_list
#define jahoda_ent_table_list

#include <jahoda/core/core.h>
#include <jahoda/gfx/render_manager.h>
#include "transform.h"

typedef struct {} dude;
typedef struct {} camera_controller;
typedef struct {} transform_updated;
typedef struct {} transform_static;
 
typedef struct {} static_geometry;
typedef struct {} volatile_geometry;

#define ent_table_list\
    X(local_transform)\
    X(global_transform)\
    X(transform_updated)\
    X(camera_controller)\
    X(dude)\
    X(render_mesh_id)\
    X(static_geometry)\
    X(volatile_geometry)\
      
#define ent_with_transform(...)\
ent_with(local_transform, .scale = {1,1,1}, __VA_ARGS__);\
ent_with(global_transform)\

#define ent_with_transform_v(...)\
ent_with_v(local_transform, __VA_ARGS__);\
ent_with(global_transform)\


#endif
