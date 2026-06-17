#include "mat.h"


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// @todo: more rich dbg_verify() reports

f64 *mat_f64_at(mat_f64* mat, uz x, uz y)
{
    dbg_verify(mat->rows > x && mat->columns > y, "out of bounds matrix index mat->rows %zu with x: %zu, mat->colums: %zu with y: %zu", mat->rows, x, mat->columns, y);
    return &mat_f64_uat(mat, x, y);
}

bool8 mat_f64_dot_compatible(const mat_f64 *mat1, const mat_f64 *mat2)
{
    return mat1->columns == mat2->rows;
}

bool8 mat_f64_comatible(const mat_f64 *mat1, const mat_f64 *mat2)
{
    return mat1->columns == mat2->columns && mat1->rows == mat2->rows;
}

void mat_f64_dump(const mat_f64 *mat)
{
    for(uz row = 0; row < mat->rows; row++)
    {
        for(uz column = 0; column < mat->columns; column++)
        {
            printf("%.2lf ", mat->data[row * mat->columns + column]);
        }
        printf("\n");
    }
}

mat_f64 mat_f64_make(arena *mem, uz rows, uz columns, bool8 zero)
{    
    return (mat_f64){
        .data = arena_push(mem, rows * columns * sizeof(f64), jahoda_alignof(f64), zero), 
        .rows = rows,
        .columns = columns
    }; 
}

mat_f64 mat_f64_make_view(mat_f64 *mat, uz rows, uz columns, uz off_rows, uz off_columns)
{
    return (mat_f64){
        .data = mat_f64_at(mat, off_rows, off_columns), 
        .rows = rows,
        .columns = columns
    }; 
}

mat_f64 mat_f64_make_rand(arena *mem, uz rows, uz columns, f64 from, f64 to)
{
    mat_f64 out = mat_f64_make(mem, rows, columns, false);

    for(uz row = 0; row < out.rows; row++)
    {
        for(uz column = 0; column < out.columns; column++)
        {
            out.data[row * out.columns + column] = ((f64)rand() / RAND_MAX) * (to - from) + from;
        }
    }

    return out;
}

mat_f64 mat_f64_make_identity(arena *mem, uz extent)
{
    mat_f64 out = mat_f64_make(mem, extent, extent, true);

    for(uz i = 0; i < extent; i++)
    {
        mat_f64_uat(&out, i, i) = 1.0;
    }

    return out;

}

mat_f64 mat_f64_copy(arena *mem, const mat_f64 *mat)
{    
    mat_f64 out = mat_f64_make(mem, mat->rows, mat->columns, false);

    memcpy(out.data, mat->data, mat->rows * mat->columns * sizeof(f64));

    return out;
}

mat_f64 mat_f64_transpose(arena *mem, const mat_f64 *mat)
{
    mat_f64 out = mat_f64_make(mem, mat->columns, mat->rows, false);
    
    for(uz row = 0; row < mat->rows; row++)
    {
        for(uz column = 0; column < mat->columns; column++)
        {
            out.data[column * mat->rows + row] = mat->data[row * mat->columns + column];
        }
    }

    return out;
}

mat_f64 mat_f64_add(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2)
{
    dbg_verify(mat_f64_comatible(mat1, mat2), "matricies are uncompatible for addition");

    mat_f64 out = mat_f64_make(mem, mat1->rows, mat1->columns, false);

    for(uz row = 0; row < mat1->rows; row++)
    {
        for(uz column = 0; column < mat1->columns; column++)
        {
            out.data[row * mat1->columns + column] = mat1->data[row * mat1->columns + column] + mat2->data[row * mat1->columns + column];
        }
    }

    return out;
}

void mat_f64_add_scaled_inplace(mat_f64 *mat1, const mat_f64 *mat2, f64 scalar)
{
    dbg_verify(mat_f64_comatible(mat1, mat2), "matricies are uncompatible for addition");

    for(uz row = 0; row < mat1->rows; row++)
    {
        for(uz column = 0; column < mat1->columns; column++)
        {
            mat1->data[row * mat1->columns + column] = mat1->data[row * mat1->columns + column] + mat2->data[row * mat1->columns + column] * scalar;
        }
    }
}

mat_f64 mat_f64_add_scaled(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2, f64 scalar)
{
    dbg_verify(mat_f64_comatible(mat1, mat2), "matricies are uncompatible for addition");

    mat_f64 out = mat_f64_make(mem, mat1->rows, mat1->columns, false);

    for(uz row = 0; row < mat1->rows; row++)
    {
        for(uz column = 0; column < mat1->columns; column++)
        {
            out.data[row * mat1->columns + column] = mat1->data[row * mat1->columns + column] + mat2->data[row * mat1->columns + column] * scalar;
        }
    }

    return out;
}

mat_f64 mat_f64_sub(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2)
{
    dbg_verify(mat_f64_comatible(mat1, mat2), "matricies are uncompatible for subtraction");

    mat_f64 out = mat_f64_make(mem, mat1->rows, mat1->columns, false);

    for(uz row = 0; row < mat1->rows; row++)
    {
        for(uz column = 0; column < mat1->columns; column++)
        {
            out.data[row * mat1->columns + column] = mat1->data[row * mat1->columns + column] - mat2->data[row * mat1->columns + column];
        }
    }

    return out;
}
mat_f64 mat_f64_mul(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2)
{
    dbg_verify(mat_f64_comatible(mat1, mat2), "matricies are uncompatible for multiplication");

    mat_f64 out = mat_f64_make(mem, mat1->rows, mat1->columns, false);

    for(uz row = 0; row < mat1->rows; row++)
    {
        for(uz column = 0; column < mat1->columns; column++)
        {
            out.data[row * mat1->columns + column] = mat1->data[row * mat1->columns + column] * mat2->data[row * mat1->columns + column];
        }
    }

    return out;
}


mat_f64 mat_f64_mul_scalar(arena *mem, const mat_f64 *mat, f64 scalar)
{
    mat_f64 out = mat_f64_copy(mem, mat);

    for(uz row = 0; row < out.rows; row++)
    {
        for(uz column = 0; column < out.columns; column++)
        {
            mat_f64_uat(&out, row, column) *= scalar;
        }
    }

    return out;
}

mat_f64 mat_f64_add_scalar(arena *mem, const mat_f64 *mat, f64 scalar)
{
    mat_f64 out = mat_f64_copy(mem, mat);

    for(uz row = 0; row < out.rows; row++)
    {
        for(uz column = 0; column < out.columns; column++)
        {
            mat_f64_uat(&out, row, column) += scalar;
        }
    }

    return out;
}


mat_f64 mat_f64_dot(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2)
{
    dbg_verify(mat_f64_dot_compatible(mat1, mat2), "matricies are uncompatible for dot product x1: %zu, y1: %zu, x2: %zu, y2: %zu", mat1->rows, mat1->columns, mat2->rows, mat2->columns);

    mat_f64 out = mat_f64_make(mem, mat1->rows, mat2->columns, false);
    
    for(uz column2 = 0; column2 < mat2->columns; column2++)
    {
        for(uz row1 = 0; row1 < mat1->rows; row1++)
        {
            f64 sum = 0.0;
            for(uz row2 = 0; row2 < mat2->rows; row2++)
            {
                sum += mat1->data[row1 * mat1->columns + row2] * mat2->data[row2 * mat2->columns + column2];
            }                  

            out.data[row1 * out.columns + column2] = sum;
        }
    }

    return out;
}

mat_f64 mat_f64_map(arena *mem, const mat_f64 *mat, f64 (*func)(f64))
{
    mat_f64 out = mat_f64_copy(mem, mat);

    mat_f64_map_inplace(&out, func);

    return out;
}

void mat_f64_map_inplace(mat_f64 *mat, f64 (*func)(f64))
{
     for(uz row = 0; row < mat->rows; row++)
    {
        for(uz column = 0; column < mat->columns; column++)
        {
            mat_f64_uat(mat, row, column) = func(mat_f64_uat(mat, row, column));
        }
    }
}


void mat_f64_swap_rows_inplace(mat_f64 *mat, uz row1, uz row2)
{
    dbg_verify(mat->rows > row1 && mat->rows > row2, "swap of out of bound row");

    for(uz column = 0; column < mat->columns; column++)
    {
        f64 temp = mat_f64_uat(mat, row1, column);
        mat_f64_uat(mat, row1, column) = mat_f64_uat(mat, row2, column);
        mat_f64_uat(mat, row2, column) = temp;
    }    
}

void mat_f64_scale_row_inplace(mat_f64 *mat, uz row, f64 scalar)
{
    dbg_verify(mat->rows > row, "scale of out of bound row");

    for(uz column = 0; column < mat->columns; column++)
    {
        mat_f64_uat(mat, row, column) *= scalar;
    }       
}

void mat_f64_softmax(mat_f64 *logits)
{
    f64 max_val = -1e9;
    for (uz i = 0; i < logits->columns; i++)
    {
        f64 val = mat_f64_uat(logits, 0, i);
        if (val > max_val) 
        {
            max_val = val;
        }
    }

    f64 sum = 0.0;
    for (uz i = 0; i < logits->columns; i++)
    {
        f64 e_x = exp(mat_f64_uat(logits, 0, i) - max_val);
        mat_f64_uat(logits, 0, i) = e_x;
        sum += e_x;
    }

    for (uz i = 0; i < logits->columns; i++)
    {
        mat_f64_uat(logits, 0, i) /= sum;
    }
}

void mat_f64_ReLU_inplace(mat_f64 *mat)
{
    for(uz row = 0; row < mat->rows; row++)
    {
        for(uz column = 0; column < mat->columns; column++)
        {
            if(mat_f64_uat(mat, row, column) <= 0.0)
            {
                mat_f64_uat(mat, row, column) = 0.0;
            }
        }
    }
}

void mat_f64_ReLU_backward_inplace(mat_f64 *grad, mat_f64 *activation)
{
    for(uz row = 0; row < grad->rows; row++)
    {
        for(uz col = 0; col < grad->columns; col++)
        {
            if (mat_f64_uat(activation, row, col) <= 0.0) 
            {
                mat_f64_uat(grad, row, col) = 0.0;
            }
        }
    }
}


void mat_f64_dump_to_file(const mat_f64 *mat, FILE *file)
{
    fwrite(&mat->rows, sizeof(mat->rows), 1, file);
    fwrite(&mat->columns, sizeof(mat->columns), 1, file);
    fwrite(mat->data, sizeof(f64), mat->rows * mat->columns, file);
}

mat_f64 mat_f64_load_from_file(arena *mem, FILE *file)
{
    mat_f64 out = {0};
    
    fread(&out.rows, sizeof(out.rows), 1, file);
    fread(&out.columns, sizeof(out.columns), 1, file);

    out.data = arena_push(mem, out.rows * out.columns * sizeof(f64), jahoda_alignof(f64),  false);
    fread(out.data, sizeof(f64), out.rows * out.columns, file);

    return out;
}