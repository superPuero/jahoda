#include "cee.h"

#include <jahoda/base/image.h>

tensor_f64 kernelize(arena *mem, const tensor_f64 *input, uz kernel_size)
{
    uz B = input->shape[0];
    uz C = input->shape[1];
    uz H = input->shape[2];
    uz W = input->shape[3];

    uz out_H = H - kernel_size + 1;
    uz out_W = W - kernel_size + 1;
    uz out_pixels = out_H * out_W;
    
    uz kernel_area = kernel_size * kernel_size;
    uz flattened_channels = C * kernel_area;

    tensor_f64 out = tensor_f64_make(
        .mem = mem, 
        .rank = 3, 
        .shape = {B, flattened_channels, out_pixels}, 
        .zero = false
    );

    for (uz b = 0; b < B; b++) 
    {
        for (uz y = 0; y < out_H; y++) 
        {
            for (uz x = 0; x < out_W; x++) 
            {
                uz patch_column = (y * out_W) + x;

                for (uz c = 0; c < C; c++) 
                {
                    for (uz ky = 0; ky < kernel_size; ky++) 
                    {
                        for (uz kx = 0; kx < kernel_size; kx++) 
                        {                           
                            uz row_in_col = (c * kernel_area) + (ky * kernel_size) + kx;

                            f64 pixel = *tensor_at(input, b, c, y + ky, x + kx);

                            *tensor_at(&out, b, row_in_col, patch_column) = pixel;
                        }
                    }
                }
            }
        }
    }

    return out;
}

tensor_f64 load_image_to_tensor(arena *static_memory, arena *temp_memory, strv path)
{
    marker m = arena_mark(temp_memory);

    image img = image_load(temp_memory, static_memory, path);

    dbg_verify(img.extent.x == 64 && img.extent.y == 64);
    dbg_verify(img.channels == 3); 

    tensor_f64 out = tensor_f64_make(.mem = static_memory, .rank = 4, .shape = {1, 1, img.extent.y, img.extent.x}, .zero = false);

    for(uz row = 0; row < out.shape[2]; row++)
    {
        for(uz column = 0; column < out.shape[3]; column++)
        {
            uz pixel_base = (row * img.extent.x * 3) + (column * 3);    
            
            f64 r = (f64)img.data[pixel_base + 0] / 255.0;
            f64 g = (f64)img.data[pixel_base + 1] / 255.0;
            f64 b = (f64)img.data[pixel_base + 2] / 255.0;
            
            f64 grayscale = (0.299 * r) + (0.587 * g) + (0.114 * b);
            
            *tensor_at(&out, 0, 0, row, column) = grayscale;
        }
    }

    image_release(&img);
    arena_pop_to_marker(m);

    return out;
}

// @think: uz original_rows, uz original_colsa can be inferred from column-matrix now, but not in fututre, maybe there is a better way to pass that data round
tensor_f64 dekernelize(arena *mem, const tensor_f64 *columns, uz orig_H, uz orig_W, uz kernel_size)
{
    uz B = columns->shape[0];
    uz flattened_channels = columns->shape[1];
    
    uz kernel_area = kernel_size * kernel_size;
    uz C = flattened_channels / kernel_area;
    
    uz out_H = orig_H - kernel_size + 1;
    uz out_W = orig_W - kernel_size + 1;

    tensor_f64 dX = tensor_f64_make(
        .mem = mem, 
        .rank = 4, 
        .shape = {B, C, orig_H, orig_W}, 
        .zero = true 
    );

    for (uz b = 0; b < B; b++) 
    {
        for (uz y = 0; y < out_H; y++) 
        {
            for (uz x = 0; x < out_W; x++) 
            {
                uz patch_column = (y * out_W) + x;

                for (uz c = 0; c < C; c++) 
                {
                    for (uz ky = 0; ky < kernel_size; ky++) 
                    {
                        for (uz kx = 0; kx < kernel_size; kx++) 
                        {
                            uz row_in_col = (c * kernel_area) + (ky * kernel_size) + kx;

                            f64 grad = *tensor_at(columns, b, row_in_col, patch_column);

                            *tensor_at(&dX, b, c, y + ky, x + kx) += grad;
                        }
                    }
                }
            }
        }
    }

    return dX;
}

pool_result max_pool_2x2(arena *mem, const tensor_f64 *input)
{
    uz B = input->shape[0];
    uz C = input->shape[1];
    uz H = input->shape[2];
    uz W = input->shape[3];
    
    uz out_H = H / 2; 
    uz out_W = W / 2; 
    
    pool_result result = {
        .pooled_output = tensor_f64_make(.mem = mem, .rank = 4, .shape = {B, C, out_H, out_W}),
        .argmax_indices = tensor_f64_make(.mem = mem, .rank = 4, .shape = {B, C, out_H, out_W})
    };
    
    for (uz b = 0; b < B; b++)
    {
        for (uz c = 0; c < C; c++)
        {
            for (uz py = 0; py < out_H; py++)
            {
                for (uz px = 0; px < out_W; px++)
                {
                    f64 max_val = -1e9; 
                    uz max_idx = -1;
                    
                    for (uz ky = 0; ky < 2; ky++)
                    {
                        for (uz kx = 0; kx < 2; kx++)
                        {
                            uz in_y = (py * 2) + ky;
                            uz in_x = (px * 2) + kx;
                            
                            uz flat_spatial_idx = (in_y * W) + in_x;
                            
                            f64 val = *tensor_at(input, b, c, in_y, in_x);
                            
                            if (val > max_val)
                            {
                                max_val = val;
                                max_idx = flat_spatial_idx; 
                            }
                        }
                    }
                    
                    *tensor_at(&result.pooled_output, b, c, py, px) = max_val;
                    *tensor_at(&result.argmax_indices, b, c, py, px) = (f64)max_idx;
                }
            }
        }
    }
    
    return result;
}
// 
tensor_f64 max_pool_backward(arena *mem, const tensor_f64 *dA_pooled, const tensor_f64 *argmax_indices, uz orig_H, uz orig_W)
{
    uz B = dA_pooled->shape[0];
    uz C = dA_pooled->shape[1];
    uz pooled_H = dA_pooled->shape[2];
    uz pooled_W = dA_pooled->shape[3];

    tensor_f64 dX = tensor_f64_make(
        .mem = mem, 
        .rank = 4, 
        .shape = {B, C, orig_H, orig_W}, 
        .zero = true
    );

    for (uz b = 0; b < B; b++)
    {
        for (uz c = 0; c < C; c++)
        {
            for (uz py = 0; py < pooled_H; py++)
            {
                for (uz px = 0; px < pooled_W; px++)
                {
                    f64 grad = *tensor_at(dA_pooled, b, c, py, px);
                    
                    uz flat_spatial_idx = (uz)(*tensor_at(argmax_indices, b, c, py, px));
                    
                    // Now safely decodes using the exact width from the forward pass
                    uz in_y = flat_spatial_idx / orig_W;
                    uz in_x = flat_spatial_idx % orig_W;
                    
                    *tensor_at(&dX, b, c, in_y, in_x) += grad;
                }
            }
        }
    }

    return dX;
}
cee_nn cee_nn_make(arena *mem, f64 learning_rate)
{
    return (cee_nn){
        .learning_rate = learning_rate,
        .conv1_weights = tensor_f64_make_rand(.mem = mem, .rank = 2, .shape = {16, 9}, .from = -0.1, .to = 0.1),
        .conv2_weights = tensor_f64_make_rand(.mem = mem, .rank = 2, .shape = {16, 144}, .from = -0.1, .to = 0.1),
        .dense1_weights = tensor_f64_make_rand(.mem = mem, .rank = 2, .shape = {16 * 14 * 14, 128}, .from = -0.1, .to = 0.1),
        .out_weights = tensor_f64_make_rand(.mem = mem, .rank = 2, .shape = {128, 10}, .from = -0.1, .to = 0.1)
    };
}
void cee_nn_train_with(cee_nn *nn, arena *temp_mem, tensor_f64 *img, uz label)
{
    uz B = img->shape[0]; 

    tensor_f64 layer1_kernelized = kernelize(temp_mem, img, 3);

    tensor_f64 conv1_out_3d = tensor_f64_mm(temp_mem, &nn->conv1_weights, &layer1_kernelized);
    
    tensor_f64 conv1_out_4d = tensor_f64_make_view(
        .tensor = &conv1_out_3d, .rank = 4, .shape = {B, 16, 62, 62}
    );

    tensor_f64_relu_inplace(&conv1_out_4d);

    pool_result r1 = max_pool_2x2(temp_mem, &conv1_out_4d);

    tensor_f64 layer2_kernelized = kernelize(temp_mem, &r1.pooled_output, 3);
    tensor_f64 conv2_out_3d = tensor_f64_mm(temp_mem, &nn->conv2_weights, &layer2_kernelized);
    
    tensor_f64 conv2_out_4d = tensor_f64_make_view(
        .tensor = &conv2_out_3d, .rank = 4, .shape = {B, 16, 29, 29}
    );

    tensor_f64_relu_inplace(&conv2_out_4d);
    pool_result r2 = max_pool_2x2(temp_mem, &conv2_out_4d);
    
    tensor_f64 flat_pool = tensor_f64_make_view(
        .tensor = &r2.pooled_output, .rank = 2, .shape = {B, 16 * 14 * 14}
    );

    tensor_f64 dense1_out = tensor_f64_mm(temp_mem, &flat_pool, &nn->dense1_weights);
    tensor_f64_relu_inplace(&dense1_out);

    tensor_f64 final_predictions = tensor_f64_mm(temp_mem, &dense1_out, &nn->out_weights);
    tensor_f64_softmax_inplace(&final_predictions);

    tensor_f64 target_vector = tensor_f64_make(.mem = temp_mem, .rank = 2, .shape = {1, 10}, .zero = true); 
    *tensor_at(&target_vector, 0, label) = 1.0;

    tensor_f64_sub_inplace(&final_predictions, &target_vector);

    tensor_f64 dense1_out_T = tensor_f64_transpose(temp_mem, &dense1_out, 0, 1);
    tensor_f64 dW_out = tensor_f64_mm(temp_mem, &dense1_out_T, &final_predictions);
    
    tensor_f64 out_weights_T = tensor_f64_transpose(temp_mem, &nn->out_weights, 0, 1);
    tensor_f64 dA_dense1 = tensor_f64_mm(temp_mem, &final_predictions, &out_weights_T);
    tensor_f64_relu_backward_inplace(&dA_dense1, &dense1_out);
    
    tensor_f64 flat_pool_T = tensor_f64_transpose(temp_mem, &flat_pool, 0, 1);
    tensor_f64 dW_dense1 = tensor_f64_mm(temp_mem, &flat_pool_T, &dA_dense1);
    
    tensor_f64 dense1_weights_T = tensor_f64_transpose(temp_mem, &nn->dense1_weights, 0, 1);
    tensor_f64 dA_flat = tensor_f64_mm(temp_mem, &dA_dense1, &dense1_weights_T);

    tensor_f64 dA_pooled2 = tensor_f64_make_view(
        .tensor = &dA_flat, .rank = 4, .shape = {B, 16, 14, 14}
    );

    tensor_f64 dA_conv2_activated = max_pool_backward(temp_mem, &dA_pooled2, &r2.argmax_indices, 29, 29);
    tensor_f64_relu_backward_inplace(&dA_conv2_activated, &conv2_out_4d);

    tensor_f64 dA_conv2_flat = tensor_f64_make_view(
        .tensor = &dA_conv2_activated, .rank = 3, .shape = {B, 16, 29 * 29}
    );

    tensor_f64 layer2_cols_T = tensor_f64_transpose(temp_mem, &layer2_kernelized, 1, 2);
    tensor_f64 dW_conv2 = tensor_f64_mm(temp_mem, &dA_conv2_flat, &layer2_cols_T);

    tensor_f64 conv2_weights_T = tensor_f64_transpose(temp_mem, &nn->conv2_weights, 0, 1);
    tensor_f64 dX_layer2_cols = tensor_f64_mm(temp_mem, &conv2_weights_T, &dA_conv2_flat);

    tensor_f64 dA_pool1 = dekernelize(temp_mem, &dX_layer2_cols, 31, 31, 3);

    tensor_f64 dA_conv1_activated = max_pool_backward(temp_mem, &dA_pool1, &r1.argmax_indices, 62, 62);
    tensor_f64_relu_backward_inplace(&dA_conv1_activated, &conv1_out_4d);

    tensor_f64 dA_conv1_flat = tensor_f64_make_view(
        .tensor = &dA_conv1_activated, .rank = 3, .shape = {B, 16, 62 * 62}
    );

    tensor_f64 layer1_cols_T = tensor_f64_transpose(temp_mem, &layer1_kernelized, 1, 2);    
    tensor_f64 dW_conv1 = tensor_f64_mm(temp_mem, &dA_conv1_flat, &layer1_cols_T);

    // tensor_f64_dump_shape(&dW_conv2);

    tensor_f64 dW_conv2_3d = tensor_f64_mm(temp_mem, &dA_conv2_flat, &layer2_cols_T);

    tensor_f64 dW_conv1_3d = tensor_f64_mm(temp_mem, &dA_conv1_flat, &layer1_cols_T);

    tensor_f64 dW_conv2_2 = tensor_f64_sum_batch_dim(temp_mem, &dW_conv2_3d);
    tensor_f64 dW_conv1_2 = tensor_f64_sum_batch_dim(temp_mem, &dW_conv1_3d);

    tensor_f64_add_scaled_inplace(&nn->out_weights, &dW_out, -nn->learning_rate);
    tensor_f64_add_scaled_inplace(&nn->dense1_weights, &dW_dense1, -nn->learning_rate);
    tensor_f64_add_scaled_inplace(&nn->conv2_weights, &dW_conv2_2, -nn->learning_rate);
    tensor_f64_add_scaled_inplace(&nn->conv1_weights, &dW_conv1_2, -nn->learning_rate); 
}

tensor_f64 cee_nn_predict(cee_nn *nn, arena *temp_mem, tensor_f64 *img)
{
    uz B = img->shape[0]; 

    tensor_f64 layer1_cols = kernelize(temp_mem, img, 3);
    tensor_f64 conv1_out_3d = tensor_f64_mm(temp_mem, &nn->conv1_weights, &layer1_cols);
    


    tensor_f64 conv1_out_4d = tensor_f64_make_view(
        .tensor = &conv1_out_3d, .rank = 4, .shape = {B, 16, 62, 62}
    );
    
    tensor_f64_relu_inplace(&conv1_out_4d);
    pool_result r1 = max_pool_2x2(temp_mem, &conv1_out_4d);

    tensor_f64 layer2_cols = kernelize(temp_mem, &r1.pooled_output, 3);
    tensor_f64 conv2_out_3d = tensor_f64_mm(temp_mem, &nn->conv2_weights, &layer2_cols);

    tensor_f64 conv2_out_4d = tensor_f64_make_view(
        .tensor = &conv2_out_3d, .rank = 4, .shape = {B, 16, 29, 29}
    );

    tensor_f64_relu_inplace(&conv2_out_4d);
    pool_result r2 = max_pool_2x2(temp_mem, &conv2_out_4d);

    tensor_f64 flat_pool = tensor_f64_make_view(
        .tensor = &r2.pooled_output, .rank = 2, .shape = {B, 16 * 14 * 14}
    );

    tensor_f64 dense1_out = tensor_f64_mm(temp_mem, &flat_pool, &nn->dense1_weights);
    tensor_f64_relu_inplace(&dense1_out);

    tensor_f64 final_predictions = tensor_f64_mm(temp_mem, &dense1_out, &nn->out_weights);
    tensor_f64_softmax_inplace(&final_predictions);

    return final_predictions;
}

// void cee_nn_train_with(cee_nn *nn, arena *temp_mem, mat_f64 *img, uz label)
// {
// 	mat_f64 target_vector = mat_f64_make(temp_mem, 1, 10, true); 

// 	mat_f64_uat(&target_vector, 0, label) = 1.0;

// 	// flattening into columns for each kernel batch 3x3
// 	mat_f64 img_columns = kernelize(temp_mem, img, 3);

// 	// get feature maps
// 	mat_f64 conv1_out = mat_f64_dot(temp_mem, &nn->conv1_weights, &img_columns);
	
//     mat_f64_ReLU_inplace(&conv1_out);
	
// 	// compress feature maps into 31 by 31
// 	pool_result r = max_pool_2x2(temp_mem, &conv1_out, 62, 62);

// 	// 16 kernels of 16 * 3 * 3 size
// 	uz in_channels = 16;
//     uz out_kernels = 16;
//     uz spatial_pixels_out = 29 * 29; 

// 	mat_f64 conv2_out = mat_f64_make(temp_mem, out_kernels, spatial_pixels_out, true);

//     for (uz out_k = 0; out_k < out_kernels; out_k++)
//     {
//         for (uz in_c = 0; in_c < in_channels; in_c++)
//         {
//             marker loop_marker = arena_mark(temp_mem);

//             mat_f64 channel_view = mat_f64_make_view(&r.pooled_output, 31, 31, in_c, 0);

//             mat_f64 layer2_columns = kernelize(temp_mem, &channel_view, 3);

//             mat_f64 kernel_view = mat_f64_make_view(&nn->conv2_weights, 1, 9, out_k, in_c * 9);

//             mat_f64 single_conv_step = mat_f64_dot(temp_mem, &kernel_view, &layer2_columns);

//             for (uz p = 0; p < spatial_pixels_out; p++)
//             {
//                 mat_f64_uat(&conv2_out, out_k, p) += mat_f64_uat(&single_conv_step, 0, p);
//             }

//             arena_pop_to_marker(loop_marker);
//         }
//     }

//     mat_f64_ReLU_inplace(&conv2_out);
// 	// mat_f64_map_inplace(&conv2_out, ReLU);

// 	// compress into 16 of 14 by 14
// 	pool_result r2 = max_pool_2x2(temp_mem, &conv2_out, 29, 29);

//     mat_f64 flat_pool_view = mat_f64_make_view(&r2.pooled_output, 1, 16 * 14 * 14, 0, 0);

//     mat_f64 dense1_out = mat_f64_dot(temp_mem, &flat_pool_view, &nn->dense1_weights);

//     mat_f64_ReLU_inplace(&dense1_out);
//     // mat_f64_map_inplace(&dense1_out, ReLU);

//     mat_f64 final_predictions = mat_f64_dot(temp_mem, &dense1_out, &nn->out_weights);

//     mat_f64_softmax(&final_predictions);

//     // derivative of Categorial Cross-Entropy is ugly, softmax aswell, but together they cancel out to minus 
// 	mat_f64 prediction_error = mat_f64_sub(temp_mem, &final_predictions, &target_vector);

// 	mat_f64 dense1_out_T = mat_f64_transpose(temp_mem, &dense1_out);
    
//     mat_f64 dW_out = mat_f64_dot(temp_mem, &dense1_out_T, &prediction_error);

//     mat_f64 out_weights_T = mat_f64_transpose(temp_mem, &nn->out_weights);
    
//     mat_f64 dA_dense1 = mat_f64_dot(temp_mem, &prediction_error, &out_weights_T);

//     mat_f64_ReLU_backward_inplace(&dA_dense1, &dense1_out);

// 	mat_f64 flat_vector_T = mat_f64_transpose(temp_mem, &flat_pool_view);
    
//     mat_f64 dW_dense1 = mat_f64_dot(temp_mem, &flat_vector_T, &dA_dense1);

//     mat_f64 dense1_weights_T = mat_f64_transpose(temp_mem, &nn->dense1_weights);
    
//     mat_f64 dA_flat = mat_f64_dot(temp_mem, &dA_dense1, &dense1_weights_T);

// 	mat_f64 dA_pooled2 = mat_f64_make_view(&dA_flat, 16, 196, 0, 0);

// 	mat_f64 dA_conv2_activated = max_pool_backward(temp_mem, &dA_pooled2, &r2.argmax_indices, 841);

// 	mat_f64_ReLU_backward_inplace(&dA_conv2_activated, &conv2_out);

// 	mat_f64 dW_conv2 = mat_f64_make(temp_mem, 16, 144, true); 

// 	mat_f64 dA_pool1 = mat_f64_make(temp_mem, 16, 961, true); 

// 	{
// 		for (int out_f = 0; out_f < 16; out_f++) 
// 		{
// 			mat_f64 error_row = mat_f64_make_view(&dA_conv2_activated, 1, 29 * 29, out_f, 0);;
	
// 			for (int in_c = 0; in_c < 16; in_c++) 
// 			{
//                 mat_f64 single_channel_view = mat_f64_make_view(&r.pooled_output, 31, 31, in_c, 0);
// 				marker loop_marker = arena_mark(temp_mem);  
	
// 				mat_f64 layer2_columns = kernelize(temp_mem, &single_channel_view, 3);
// 				mat_f64 layer2_cols_T = mat_f64_transpose(temp_mem, &layer2_columns);
// 				mat_f64 dW_kernel = mat_f64_dot(temp_mem, &error_row, &layer2_cols_T);
	
// 				for (int p = 0; p < 9; p++) 
// 				{
// 					*mat_f64_at(&dW_conv2, out_f, (in_c * 9) + p) = *mat_f64_at(&dW_kernel, 0, p);
// 				}
	
// 				mat_f64 specific_kernel;
// 				specific_kernel.rows = 1;
// 				specific_kernel.columns = 9;
// 				specific_kernel.data = &mat_f64_uat(&nn->conv2_weights, out_f, in_c * 9);
				
// 				mat_f64 specific_kernel_T = mat_f64_transpose(temp_mem, &specific_kernel);
				
// 				mat_f64 dX_columns = mat_f64_dot(temp_mem, &specific_kernel_T, &error_row);
				
// 				mat_f64 spatial_grad = dekernelize(temp_mem, &dX_columns, 31, 31, 3);
				
// 				for (int y = 0; y < 31; y++) 
// 				{
// 					for (int x = 0; x < 31; x++) 
// 					{
// 						int flat_idx = (y * 31) + x;
// 						mat_f64_uat(&dA_pool1, in_c, flat_idx) += mat_f64_uat(&spatial_grad, y, x);
// 					}
// 				}
	
// 				arena_pop_to_marker(loop_marker);
// 			}
// 		}
// 	}

// 	mat_f64 dA_conv1_activated = max_pool_backward(temp_mem, &dA_pool1, &r.argmax_indices, 3844);

//     mat_f64_ReLU_backward_inplace(&dA_conv1_activated, &conv1_out);

//     mat_f64 img_columns_T = mat_f64_transpose(temp_mem, &img_columns);    
//     mat_f64 dW_conv1 = mat_f64_dot(temp_mem, &dA_conv1_activated, &img_columns_T);

//     mat_f64_add_scaled_inplace(&nn->out_weights, &dW_out, -nn->learning_rate);
//     mat_f64_add_scaled_inplace(&nn->dense1_weights, &dW_dense1, -nn->learning_rate);
//     mat_f64_add_scaled_inplace(&nn->conv2_weights, &dW_conv2, -nn->learning_rate);
//     mat_f64_add_scaled_inplace(&nn->conv1_weights, &dW_conv1, -nn->learning_rate);   
// }

// mat_f64 cee_nn_predict(cee_nn *nn, arena *temp_mem, mat_f64 *img)
// {
//     mat_f64 img_columns = kernelize(temp_mem, img, 3);

//     mat_f64 conv1_out = mat_f64_dot(temp_mem, &nn->conv1_weights, &img_columns);

//     mat_f64_ReLU_inplace(&conv1_out);

//     pool_result r1 = max_pool_2x2(temp_mem, &conv1_out, 62, 62);

//     mat_f64 conv2_out = mat_f64_make(temp_mem, 16, 841, true); 
//     mat_f64 single_channel_view;
//     single_channel_view.rows = 31;
//     single_channel_view.columns = 31;

//     for (int out_f = 0; out_f < 16; out_f++) 
//     {
//         for (int in_c = 0; in_c < 16; in_c++) 
//         {
//             single_channel_view.data = &mat_f64_uat(&r1.pooled_output, in_c, 0);
//             mat_f64 layer2_columns = kernelize(temp_mem, &single_channel_view, 3);
            
//             mat_f64 specific_kernel;
//             specific_kernel.rows = 1;
//             specific_kernel.columns = 9;
//             specific_kernel.data = &mat_f64_uat(&nn->conv2_weights, out_f, in_c * 9);

//             mat_f64 partial_conv = mat_f64_dot(temp_mem, &specific_kernel, &layer2_columns);

//             for (int p = 0; p < 841; p++) {
//                 mat_f64_uat(&conv2_out, out_f, p) += mat_f64_uat(&partial_conv, 0, p);
//             }
//         }
//     }
//     mat_f64_ReLU_inplace(&conv2_out);
//     pool_result r2 = max_pool_2x2(temp_mem, &conv2_out, 29, 29);

//     mat_f64 flat_vector;
//     flat_vector.rows = 1;
//     flat_vector.columns = 16 * 196;
//     flat_vector.data = r2.pooled_output.data;

//     mat_f64 dense1_out = mat_f64_dot(temp_mem, &flat_vector, &nn->dense1_weights);
//     mat_f64_ReLU_inplace(&dense1_out);

//     mat_f64 final_predictions = mat_f64_dot(temp_mem, &dense1_out, &nn->out_weights);
//     // mat_f64_softmax(&final_predictions);

//     return final_predictions;
// }

void cee_nn_dump_to_file(cee_nn *nn,arena *temp_mem, strv path)
{
    str path_nt = str_from_view_nt(temp_mem, path); 

    FILE *file = fopen(path_nt.data, "wb");

    verify(file, "unable to open file");

    fwrite(&nn->learning_rate, sizeof(f64), 1, file);
    tensor_f64_dump_to_file(&nn->conv1_weights, file);
    tensor_f64_dump_to_file(&nn->conv2_weights, file);  
    tensor_f64_dump_to_file(&nn->dense1_weights, file); 
    tensor_f64_dump_to_file(&nn->out_weights, file);
}

cee_nn cee_nn_load_from_file(arena *mem, arena *temp_mem, strv path)
{
    cee_nn nn = {0};

    str path_nt = str_from_view_nt(temp_mem, path); 

    FILE *file = fopen(path_nt.data, "rb");

    verify(file, "unable to open file");

    fread(&nn.learning_rate, sizeof(f64), 1, file);
    nn.conv1_weights = tensor_f64_load_from_file(mem, file);
    nn.conv2_weights = tensor_f64_load_from_file(mem, file);  
    nn.dense1_weights = tensor_f64_load_from_file(mem, file); 
    nn.out_weights = tensor_f64_load_from_file(mem, file);   

    return nn;
}