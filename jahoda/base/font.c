#include "font.h"
#include <jahoda/core/file.h>

font_atlas font_atlas_make(arena *static_arena, arena *temp_arena, strv font_path)
{
    font_atlas out = {0};
    out.atlas_width = 512;  
    out.atlas_height = 512;
    out.font_height = 96;
    out.atlas_pixels = arena_push(static_arena, out.atlas_width  *out.atlas_height, 1, false);

    str ttf_buffer = file_load(temp_arena, font_path);
    verifyl(font_loading_fail, ttf_buffer.data != NULL, "failed to load font file");

    i32 result = stbtt_BakeFontBitmap(
        (const u8*)ttf_buffer.data, 0,                       
        96,                       
        out.atlas_pixels,                     
        out.atlas_width, out.atlas_height,    
        32, 96,                               
        out.glyphs                            
    );

    out.atlas_pixels[out.atlas_width  *0 + 0] = 255;

    verifyl(font_loading_fail, result > 0, "font atlas is not big enough to fit all characters");

    return out;
}

f32 font_measure_text_width(font_atlas *font, strv text)
{
    f32 total_width = 0.0f;

    for (uz i = 0; i < text.len; i++)
    {
        u8 character = text.data[i];

        if (character < 32 || character >= 128) {
            continue; 
        }

        u32 glyph_index = character - 32;

        f32 advance = font->glyphs[glyph_index].xadvance; 

        total_width += advance;
    }

    return total_width;
}

text_vertex_buffer generate_text_vertices(arena *mem, font_atlas *font, strv text, f32 start_x, f32 start_y)
{
	text_vertex_buffer out = {0};
	da_reserve(mem, &out, text.len  *6);

    f32 cursor_x = start_x;
    f32 cursor_y = start_y;

	da_reserve(mem, &out, 6  *text.len);

	for(uz ch = 0; ch < text.len; ch++) 
    {
        unsigned char c = text.data[ch];
        
        if (c >= 32 && c < 128) 
        {
            stbtt_aligned_quad q;
            
            // stbtt automatically updates cursor_x and cursor_y based on the char's advance!
            // The '1' at the end is an OpenGL flag. Passing 1 aligns the quads properly 
            // for standard top-left origin rendering.
            stbtt_GetBakedQuad(
                font->glyphs, 
                font->atlas_width, 
                font->atlas_height, 
                c - 32,        // Index into the glyph array
                &cursor_x, 
                &cursor_y, 
                &q, 
                1
            );

            // q.x0, q.y0 is Top-Left screen pixel coordinate
            // q.s0, q.t0 is Top-Left UV coordinate

            // Triangle 1 (Top-Left, Bottom-Left, Bottom-Right)
			da_append(mem, &out, (text_vertex){{q.x0, q.y0}, {q.s0, q.t0}});
			da_append(mem, &out, (text_vertex){{q.x0, q.y1}, {q.s0, q.t1}});
			da_append(mem, &out, (text_vertex){{q.x1, q.y1}, {q.s1, q.t1}});
			da_append(mem, &out, (text_vertex){{q.x0, q.y0}, {q.s0, q.t0}});
			da_append(mem, &out, (text_vertex){{q.x1, q.y1}, {q.s1, q.t1}});
			da_append(mem, &out, (text_vertex){{q.x1, q.y0}, {q.s1, q.t0}});
        }
    }

	return out;
}