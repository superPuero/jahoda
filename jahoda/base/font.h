#ifndef jahoda_font
#define jahoda_font

#include <stb_truetype.h>
#include <jahoda/core/types.h>
#include <jahoda/core/arena.h>
#include <jahoda/core/math.h>
#include <jahoda/core/da.h>

typedef struct {
    u32 font_height;
    u8 *atlas_pixels;
    u32 atlas_width;
    u32 atlas_height;
    stbtt_bakedchar glyphs[96];
} font_atlas;

da_declare(font_atlas, font_atlas_da);

typedef struct {
	vec2_f32 pos;
    vec2_f32 uv;				
} text_vertex;

da_declare(text_vertex, text_vertex_buffer);

f32 font_measure_text_width(font_atlas *font, strv text);   
font_atlas font_atlas_make(arena *static_arena, arena *temp_arena, strv font_path);
text_vertex_buffer generate_text_vertices(arena *mem, font_atlas *font, strv text, f32 start_x, f32 start_y);

#endif