#ifndef jahoda_tensor
#define jahoda_tensor

#include "arena.h"

typedef struct
{
    f64 *data;
    u32 rank;
    u32 shape[8];
    u32 stride[8];
    u32 size;
} tensor_f64;

typedef struct
{
    arena *mem;
    u32 shape[8];
    u8 rank;         
    bool8 zero;
} tensor_config;

typedef struct
{
    tensor_f64 *tensor;
    u32 shape[8];
    u8 rank;         
} tensor_view_config;

typedef struct
{
    arena *mem;
    u32 shape[8];
    u8 rank;
    f64 from;
    f64 to;         
} tensor_config_rand;

typedef struct
{
    u32 value[8];
} tensor_index;


tensor_f64 _tensor_f64_make(tensor_config cfg);
tensor_f64 _tensor_f64_make_rand(tensor_config_rand cfg);
tensor_f64 _tensor_f64_make_view(tensor_view_config cfg);

tensor_f64 tensor_f64_copy(arena *mem, const tensor_f64 *tensor);


f64 *_tensor_f64_at(tensor_f64 *tensor, tensor_index index);
void tensor_f64_dump(tensor_f64 *tensor);
void tensor_f64_dump_shape(tensor_f64 *tensor);

bool8 tensor_f64_size_compatible(const tensor_f64 *t1, const tensor_f64 *t2);
bool8 tensor_f64_dot_compatible(const tensor_f64 *t1, const tensor_f64 *t2);

void tensor_f64_add_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor);

void tensor_f64_add_scaled_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor, f64 scalar);
tensor_f64 tensor_f64_sum_batch_dim(arena *mem, const tensor_f64 *t);
void tensor_f64_sub_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor);
void tensor_f64_mul_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor);
tensor_f64 tensor_f64_dot(arena *mem, const tensor_f64 *t1, const tensor_f64 *t2);
tensor_f64 tensor_f64_mm(arena *mem, const tensor_f64 *t1, const tensor_f64 *t2);
tensor_f64 tensor_f64_mm_2x3(arena *mem, const tensor_f64 *t1, const tensor_f64 *t2);
tensor_f64 tensor_f64_mm2(arena *mem, const tensor_f64 *t1, const tensor_f64 *t2);
tensor_f64 tensor_f64_mm3(arena *mem, const tensor_f64 *t1, const tensor_f64 *t2);

void tensor_f64_relu_inplace(tensor_f64 *tensor);
void tensor_f64_relu_backward_inplace(tensor_f64 *grad, tensor_f64 *activation);
void tensor_f64_softmax_inplace(tensor_f64 *tensor);

void tensor_f64_transpose_inplace(tensor_f64 *tensor, uz dim1, uz dim2);
tensor_f64 tensor_f64_transpose(arena *mem, const tensor_f64 *tensor, uz dim1, uz dim2);

void tensor_f64_dump_to_file(const tensor_f64 *tensor, FILE *file);
tensor_f64 tensor_f64_load_from_file(arena *mem, FILE *file);

#define tensor_f64_make(...) _tensor_f64_make((tensor_config){.zero = false, __VA_ARGS__})
#define tensor_f64_make_view(...) _tensor_f64_make_view((tensor_view_config){__VA_ARGS__})
#define tensor_f64_make_rand(...) _tensor_f64_make_rand((tensor_config_rand){.from = 1.0, .to = 1.0, __VA_ARGS__})
#define tensor_at(tensor, ...) _tensor_f64_at(tensor, (tensor_index){.value = {__VA_ARGS__}})

#endif jahoda_tensor