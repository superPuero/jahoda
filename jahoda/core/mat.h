#ifndef jahoda_mat
#define jahoda_mat

#include "types.h"
#include "arena.h"

typedef struct
{
    f64 *data;
    uz rows;
    uz columns;
} mat_f64;

#define mat_f64_uat(mat, x, y) (mat)->data[x  * (mat)->columns + y]
f64 *mat_f64_at(mat_f64* mat, uz x, uz y);

bool8 mat_f64_dot_compatible(const mat_f64 *mat1, const mat_f64 *mat2);
bool8 mat_f64_comatible(const mat_f64 *mat1, const mat_f64 *mat2);

mat_f64 mat_f64_make(arena *mem, uz rows, uz columns, bool8 zero);
mat_f64 mat_f64_make_rand(arena *mem, uz rows, uz columns, f64 from, f64 to);
mat_f64 mat_f64_make_identity(arena *mem, uz extent);
mat_f64 mat_f64_copy(arena *mem, const mat_f64 *mat);

mat_f64 mat_f64_make_view(mat_f64 *mat, uz rows, uz columns, uz off_rows, uz off_columns);

mat_f64 mat_f64_transpose(arena *mem, const mat_f64 *mat);

void mat_f64_dump(const mat_f64 *mat);

mat_f64 mat_f64_add(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2);
mat_f64 mat_f64_add_scaled(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2, f64 scalar);
void mat_f64_add_scaled_inplace(mat_f64 *mat1, const mat_f64 *mat2, f64 scalar);

mat_f64 mat_f64_sub(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2);
mat_f64 mat_f64_mul(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2);
mat_f64 mat_f64_dot(arena *mem, const mat_f64 *mat1, const mat_f64 *mat2);
mat_f64 mat_f64_mul_scalar(arena *mem, const mat_f64 *mat, f64 scalar);
mat_f64 mat_f64_add_scalar(arena *mem, const mat_f64 *mat, f64 scalar);
mat_f64 mat_f64_map(arena *mem, const mat_f64 *mat, f64 (*func)(f64));

void mat_f64_dump_to_file(const mat_f64 *mat, FILE *file);
mat_f64 mat_f64_load_from_file(arena *mem, FILE *file);

void mat_f64_softmax(mat_f64 *logits);

void mat_f64_ReLU_inplace(mat_f64 *mat);
void mat_f64_ReLU_backward_inplace(mat_f64 *grad, mat_f64 *activation);

void mat_f64_map_inplace(mat_f64 *mat, f64 (*func)(f64));

void mat_f64_swap_rows_inplace(mat_f64 *mat, uz row1, uz row2);
void mat_f64_scale_row_inplace(mat_f64 *mat, uz row, f64 scalar);

#endif 