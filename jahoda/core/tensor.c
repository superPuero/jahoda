#include "tensor.h"

#include <math.h>

tensor_f64 _tensor_f64_make(tensor_config cfg)
{
    tensor_f64 out = {
        .rank = cfg.rank
    };

    memcpy(out.shape, cfg.shape, sizeof(cfg.shape));

    out.size = 1;

    for(uz i = 0; i < out.rank; i++)
    {
        uz reverse_idx = (out.rank - 1) - i;
        out.stride[reverse_idx] = out.size;
        out.size *= out.shape[reverse_idx];
    }
    out.data = arena_push(cfg.mem, out.size * sizeof(f64), jahoda_alignof(f64), cfg.zero);
    return out;
}

tensor_f64 _tensor_f64_make_rand(tensor_config_rand cfg)
{
    tensor_config cfg_out = {
        .mem = cfg.mem,
        .rank = cfg.rank,
        .zero = false
    };

    memcpy(&cfg_out.shape, &cfg.shape, sizeof(cfg.shape));

    tensor_f64 out = _tensor_f64_make(cfg_out);

    for(uz i = 0; i < out.size; i++)
    {
        out.data[i] = ((f64)rand() / RAND_MAX) * (cfg.to - cfg.from) + cfg.from;
    }

    return out;
}

bool8 tensor_f64_verify_index(tensor_f64 *tensor, tensor_index index)
{
    return  (tensor->stride[0] * index.value[0] + 
            tensor->stride[1] * index.value[1] +
            tensor->stride[2] * index.value[2] +
            tensor->stride[3] * index.value[3] +
            tensor->stride[4] * index.value[4] +
            tensor->stride[5] * index.value[5] +
            tensor->stride[6] * index.value[6] +
            tensor->stride[7] * index.value[7]) < tensor->size;
}

f64 *_tensor_f64_at(tensor_f64 *tensor, tensor_index index)
{    
    dbg_verify(tensor_f64_verify_index(tensor, index), "invalid tensor index");

    return tensor->data + ( tensor->stride[0] * index.value[0] + 
                            tensor->stride[1] * index.value[1] +
                            tensor->stride[2] * index.value[2] +
                            tensor->stride[3] * index.value[3] +
                            tensor->stride[4] * index.value[4] +
                            tensor->stride[5] * index.value[5] +
                            tensor->stride[6] * index.value[6] +
                            tensor->stride[7] * index.value[7]);
}

tensor_f64 tensor_f64_copy(arena mem, const tensor_f64 *tensor)
{
    tensor_f64 out = *tensor;

    out.data = arena_push(mem, out.size * sizeof(f64), jahoda_alignof(f64), false);

    memcpy(out.data, tensor->data, out.size * sizeof(f64));

    return out;
}

tensor_f64 _tensor_f64_make_view(tensor_view_config cfg)
{
    tensor_f64 out = *cfg.tensor;

    out.rank = cfg.rank;    

    memcpy(out.shape, cfg.shape, sizeof(cfg.shape));

    out.size = 1;

    for(uz i = 0; i < out.rank; i++)
    {
        uz reverse_idx = (out.rank - 1) - i;
        out.stride[reverse_idx] = out.size;
        out.size *= out.shape[reverse_idx];
    }

    return out;
}

void tensor_f64_transpose_inplace(tensor_f64 *tensor, uz dim1, uz dim2) 
{
    uz temp_shape = tensor->shape[dim1];
    tensor->shape[dim1] = tensor->shape[dim2];
    tensor->shape[dim2] = temp_shape;

    uz temp_stride = tensor->stride[dim1];
    tensor->stride[dim1] = tensor->stride[dim2];
    tensor->stride[dim2] = temp_stride;
}

tensor_f64 tensor_f64_transpose(arena mem, const tensor_f64 *tensor, uz dim1, uz dim2)
{
    tensor_f64 out = tensor_f64_copy(mem, tensor);

    tensor_f64_transpose_inplace(&out, dim1, dim2);

    return out;
}
void tensor_f64_softmax_inplace(tensor_f64 *tensor)
{
    uz B = tensor->shape[0]; 
    uz C = tensor->shape[1];

    for (uz b = 0; b < B; b++)
    {
        f64 max_val = -1e9;
        for (uz i = 0; i < C; i++)
        {
            f64 val = *tensor_at(tensor, b, i);
            if (val > max_val) 
            {
                max_val = val;
            }
        }

        f64 sum = 0.0;
        for (uz i = 0; i < C; i++)
        {
            f64 e_x = exp(*tensor_at(tensor, b, i) - max_val);
            *tensor_at(tensor, b, i) = e_x;
            sum += e_x;
        }

        for (uz i = 0; i < C; i++)
        {
            *tensor_at(tensor, b, i) /= sum;
        }
    }
}
void tensor_f64_relu_inplace(tensor_f64 *tensor)
{
    for(uz i = 0; i < tensor->size; i++)
    {
        if(tensor->data[i] < 0.0)
        {
            tensor->data[i] = 0.0;
        }
    }
}

void tensor_f64_relu_backward_inplace(tensor_f64 *grad, tensor_f64 *activation)
{
    for(uz i = 0; i < grad->size; i++)
    {
        if(activation->data[i] <= 0.0)
        {
            grad->data[i] = 0.0;
        }
    }
}

void tensor_f64_dump_helper(tensor_f64 *tensor, uz rank, tensor_index index)
{
    if(rank == tensor->rank)
    {
        printf("%.2lf", *_tensor_f64_at(tensor, index));
        printf((index.value[rank] == tensor->shape[tensor->rank])? "" : ", ");
    }
    else
    {
        printf("[");
        for(uz i = 0; i < tensor->shape[rank]; i++)
        {
            tensor_f64_dump_helper(tensor, rank + 1, index);  
            index.value[rank]++;

            printf((i == tensor->shape[rank] - 1)? "" : ", ");
            
        }
        printf("]");
    }

}

void tensor_f64_dump(tensor_f64 *tensor)
{
    tensor_f64_dump_helper(tensor, 0, (tensor_index){0});
    printf("\n");
}

void tensor_f64_dump_shape(tensor_f64 *tensor)
{
    printf("[");
    for(uz i = 0; i < tensor->rank; i++)
    {
        printf("%zu", tensor->shape[i]);
        printf(i == tensor->rank - 1 ? "" : ", ");
    };
    printf("]\n");

}

bool8 tensor_f64_size_compatible(const tensor_f64 *t1, const tensor_f64 *t2)
{
    return t1->size == t2->size;
}

bool8 tensor_f64_dot_compatible(const tensor_f64 *t1, const tensor_f64 *t2)
{
    if (t1->rank < 2 || t2->rank < 2) return false;

    return t1->shape[t1->rank - 1] == t2->shape[t2->rank - 2];
}

bool8 tensor_f64_bmm_compatible(tensor_f64 *t1, tensor_f64 *t2) 
{
    if (!tensor_f64_dot_compatible(t1, t2)) 
    {
        return false;
    }

    if (t1->rank != t2->rank) 
    {
        return false; 
    }

    for (uz i = 0; i < t1->rank - 2; i++) 
    {
        if (t1->shape[i] != t2->shape[i]) 
        {
            return false;
        }
    }

    return true;
}

void tensor_f64_add_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor)
{
    for(uz i = 0; i < dest_tensor->size; i++)
    {
        dest_tensor->data[i] += add_tensor->data[i];
    }
}

tensor_f64 tensor_f64_sum_batch_dim(arena mem, const tensor_f64 *t)
{
    uz B = t->shape[0];
    uz slice_size = t->size / B;

    tensor_f64 out = *t;
    out.rank = t->rank - 1;
    
    for (uz i = 0; i < out.rank; i++) 
    {
        out.shape[i] = t->shape[i + 1];
        out.stride[i] = t->stride[i + 1];
    }
    
    out.size = slice_size;
    
    out.data = arena_push(mem, out.size * sizeof(f64), jahoda_alignof(f64), true);

    for (uz b = 0; b < B; b++) 
    {
        uz batch_offset = b * slice_size;
        for (uz i = 0; i < slice_size; i++) 
        {
            out.data[i] += t->data[batch_offset + i];
        }
    }

    return out;
}

void tensor_f64_add_scaled_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor, f64 scalar)
{            
    for(uz i = 0; i < dest_tensor->size; i++)
    {
        dest_tensor->data[i] += add_tensor->data[i] * scalar;
    }
}

void tensor_f64_sub_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor)
{
    for(uz i = 0; i < dest_tensor->size; i++)
    {
        dest_tensor->data[i] -= add_tensor->data[i];
    }
}

void tensor_f64_mul_inplace(tensor_f64 *dest_tensor, const tensor_f64 *add_tensor)
{
    for(uz i = 0; i < dest_tensor->size; i++)
    {
        dest_tensor->data[i] *= add_tensor->data[i];
    }
}

tensor_f64 tensor_f64_mm2(arena mem, const tensor_f64 *t1, const tensor_f64 *t2)
{    
    uz t1_rows = t1->shape[t1->rank - 2]; 
    uz t1_columns = t1->shape[t1->rank - 1]; 
    uz t2_columns = t2->shape[t2->rank - 1];

    tensor_f64 out = tensor_f64_make(.mem = mem, .rank = 2, .shape = {t1_rows, t2_columns});

    for(uz t1_row = 0; t1_row < t1_rows; t1_row++)
    {
        for(uz t2_column = 0; t2_column < t2_columns; t2_column++)
        {
            f64 sum = 0.0f;
            for(uz t1_column = 0; t1_column < t1_columns; t1_column++)
            {
                sum += *tensor_at(t1, t1_row, t1_column) * *tensor_at(t2, t1_column, t2_column);
            }

            *tensor_at(&out, t1_row, t2_column) = sum;
        }
    }

    return out;
}

tensor_f64 tensor_f64_dot(arena mem, const tensor_f64 *t1, const tensor_f64 *t2)
{
    return tensor_f64_mm2(mem, t1, t2);    
}

tensor_f64 tensor_f64_mm3(arena mem, const tensor_f64 *t1, const tensor_f64 *t2)
{    
    uz batches = t1->shape[t1->rank - 3]; 
    uz t1_rows = t1->shape[t1->rank - 2]; 
    uz t1_columns = t1->shape[t1->rank - 1]; 
    uz t2_columns = t2->shape[t2->rank - 1];

    tensor_f64 out = tensor_f64_make(.mem = mem, .rank = 3, .shape = {batches, t1_rows, t2_columns});

    for(uz batch = 0; batch < batches; batch++)
    {
        for(uz t1_row = 0; t1_row < t1_rows; t1_row++)
        {
            for(uz t2_column = 0; t2_column < t2_columns; t2_column++)
            {   
                f64 sum = 0.0f;
                for(uz t1_column = 0; t1_column < t1_columns; t1_column++)
                {                    
                    sum += *tensor_at(t1, batch, t1_row, t1_column) * *(tensor_at(t2, batch, t1_column, t2_column));
                }
    
                *(tensor_at(&out, batch, t1_row, t2_column)) = sum;
            }
        }
    }

    return out;
}

tensor_f64 tensor_f64_mm_2x3(arena mem, const tensor_f64 *t1, const tensor_f64 *t2)
{
    uz batches = t2->shape[0];
    uz t1_rows = t1->shape[0];
    uz t1_columns = t1->shape[1];
    uz t2_columns = t2->shape[2];

    tensor_f64 out = tensor_f64_make(.mem = mem, .rank = 3, .shape = {batches, t1_rows, t2_columns}, .zero = false);

    for(uz batch = 0; batch < batches; batch++)
    {
        for(uz t1_row = 0; t1_row < t1_rows; t1_row++)
        {
            for(uz t2_column = 0; t2_column < t2_columns; t2_column++)
            {
                f64 sum = 0.0;
                for(uz t1_column = 0; t1_column < t1_columns; t1_column++)
                {
                    sum += *tensor_at(t1, t1_row, t1_column) * *tensor_at(t2, batch, t1_column, t2_column);
                }
                *tensor_at(&out, batch, t1_row, t2_column) = sum;
            }
        }
    }

    return out;
}

tensor_f64 tensor_f64_mm(arena mem, const tensor_f64 *t1, const tensor_f64 *t2)
{
    if (t1->rank == 2 && t2->rank == 2) 
    {
        return tensor_f64_mm2(mem, t1, t2);
    }
    else if (t1->rank == 3 && t2->rank == 3) 
    {
        return tensor_f64_mm3(mem, t1, t2);
    }
    else if (t1->rank == 2 && t2->rank == 3) 
    {
        return tensor_f64_mm_2x3(mem, t1, t2);
    }
    
    dbg_verify(false, "Unsupported matrix multiplication rank combination");
    return (tensor_f64){0};
}

void tensor_f64_dump_to_file(const tensor_f64 *tensor, FILE *file)
{
    fwrite(&tensor->rank, sizeof(tensor->rank), 1, file);
    fwrite(&tensor->size, sizeof(tensor->size), 1, file);
    fwrite(&tensor->shape, sizeof(tensor->shape), 1, file);
    fwrite(&tensor->stride, sizeof(tensor->stride), 1, file);
    fwrite(tensor->data, sizeof(f64), tensor->size, file);
}

tensor_f64 tensor_f64_load_from_file(arena mem, FILE *file)
{
    tensor_f64 out = {0};
    
    fread(&out.rank, sizeof(out.rank), 1, file);
    fread(&out.size, sizeof(out.size), 1, file);
    fread(&out.shape, sizeof(out.shape), 1, file);
    fread(&out.stride, sizeof(out.stride), 1, file);
    
    out.data = arena_push(mem, out.size * sizeof(f64), jahoda_alignof(f64),  false);
    fread(out.data, sizeof(f64), out.size,  file);

    return out;
}