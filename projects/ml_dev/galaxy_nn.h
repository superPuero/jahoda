#ifndef galaxy_nn
#define galaxy_nn

#include <jahoda/core/core.h>

#define galaxy_nn_

typedef enum
{
    celestial_type_star = 0,
    celestial_type_quasar = 1,
    celestial_type_galaxy = 2,
    celestial_type_count
} celestial_type;

#define celestial_features\
    XX(f64, obj_ID)\
    XX(f64, alpha)\
    XX(f64, delta)\
    XX(f64, u)\
    XX(f64, g)\
    XX(f64, r)\
    XX(f64, i)\
    XX(f64, z)\
    XX(f64, run_ID)\
    XX(f64, rerun_ID)\
    XX(f64, cam_col)\
    XX(f64, field_ID)\
    XX(f64, spec_obj_ID)\
    XX(celestial_type, class)\
    XX(f64, redshift)\
    XX(f64, plate)\
    XX(f64, MJD)\
    XX(f64, fiber_ID)

#define celestial_features_use\
    XX(0, u)\
    XX(1, g)\
    XX(2, r)\
    XX(3, i)\
    XX(4, z)\
    XX(5, redshift)    

#define celestial_feature_count 6

typedef struct 
{
    #define XX(type, feat) type feat;
    celestial_features
    #undef XX

    
} celestial_entry;

da_declare(celestial_entry, celestial_da);

typedef struct
{
    celestial_da celestials;
} galaxy_dataset;

typedef struct 
{
    f64 learning_rate;
    f64 weights[3][6];
    f64 biases[3];
} galaxy_nn_model;




f64 softmax(galaxy_dataset *df, galaxy_nn_model *model, celestial_type type, uz entry);
galaxy_dataset galaxy_dataset_from_csv(arena *static_arena, arena *temp_arena, strv csv_path);
void galaxy_nn_train_round(galaxy_dataset *df, galaxy_nn_model *model);

#endif galaxy_nn