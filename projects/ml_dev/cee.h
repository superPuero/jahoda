#ifndef cee
#define cee

#include <jahoda/core/core.h>
#include <jahoda/core/tensor.h>

typedef struct
{
    f64 learning_rate;
    tensor_f64 conv1_weights;  //              16 x 3 x 3
    tensor_f64 conv2_weights;  //         16 x 16 x 3 x 3
    tensor_f64 dense1_weights; //  3136 x 16 * 16
    tensor_f64 out_weights;    //         16 * 16  x 10
} cee_nn;

cee_nn cee_nn_make(arena *mem, f64 learning_rate);

void cee_nn_train_with(cee_nn *nn, arena *temp_mem, tensor_f64 *img, uz label);
tensor_f64 cee_nn_predict(cee_nn *nn, arena *temp_mem, tensor_f64 *img);
void cee_nn_dump_to_file(cee_nn *nn, arena *temp_mem, strv path);
cee_nn cee_nn_load_from_file(arena *mem, arena *temp_mem, strv path);

tensor_f64 kernelize(arena *mem, const tensor_f64 *input, uz kernel_size);
tensor_f64 load_image_to_tensor(arena *static_memory, arena *temp_memory, strv path);

typedef struct 
{
    tensor_f64 pooled_output;
    tensor_f64 argmax_indices; 
} pool_result;
pool_result max_pool_2x2(arena *mem, const tensor_f64 *input);
tensor_f64 max_pool_backward(arena *mem, const tensor_f64 *dA_pooled, const tensor_f64 *argmax_indices, uz orig_H, uz orig_W);

#endif cee