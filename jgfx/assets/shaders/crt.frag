#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform texture2D scene_tex;
// layout(set = 0, binding = 1) uniform

layout(push_constant) uniform PushConstants {
    vec2 resolution;      
    float time;           
    float curvature;      
    float scanline_weight;
    float mask_strength;  
    float aberration;     
    float bloom_boost;    
} pc;

vec2 warp_uv(vec2 uv, float amount) 
{
    vec2 centered = uv * 2.0 - 1.0;
    vec2 offset = centered.yx / 5.0;
    centered = centered + centered * offset * offset * amount * 10.0;
    return centered * 0.5 + 0.5;
}

float crt_bezel(vec2 uv, float corner_softness) 
{
    vec2 d = min(uv, 1.0 - uv);
    vec2 edge = smoothstep(vec2(0.0), vec2(corner_softness), d);
    return edge.x * edge.y;
}

void main() 
{
    vec2 uv = warp_uv(in_uv, pc.curvature);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) 
    {
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 2. Radial Chromatic Aberration (stronger near screen corners, zero at center)
    vec2 center_dist = uv - 0.5;
    vec2 ca_offset = center_dist * pc.aberration;
    vec2 px = uv * vec2(pc.resolution);
    float r = texelFetch(scene_tex, ivec2(px + ca_offset * vec2(pc.resolution)), 0).r;
    float g = texelFetch(scene_tex, ivec2(px), 0).g;
    float b = texelFetch(scene_tex, ivec2(px - ca_offset * vec2(pc.resolution)), 0).b;
    vec3 color = vec3(r, g, b);

    // 3. Cheap 4-tap local phosphor halation (gives bright UI/highlights that Balatro glow)    
   ivec2 p = ivec2(px);
    vec3 glow = (
        texelFetch(scene_tex, p + ivec2( 1,  1), 0).xyz +
        texelFetch(scene_tex, p + ivec2(-1,  1), 0).xyz +
        texelFetch(scene_tex, p + ivec2( 1, -1), 0).xyz +
        texelFetch(scene_tex, p + ivec2(-1, -1), 0).xyz
    ) * 0.25;
    color = mix(color, max(color, glow), 0.25);

    // 4. Balatro Virtual CRT Resolution (keeps scanlines chunky even at 1440p/4K)
    float virtual_rows = 360.0; 
    float scan_phase = uv.y * virtual_rows * 3.14159265 * 2.0 - pc.time * 2.5;
    
    // Bright pixels bleed over scanlines (authentic beam widening)
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    float dynamic_weight = pc.scanline_weight * mix(1.0, 0.35, clamp(luma, 0.0, 1.0));
    
    float scanline = 1.0 - dynamic_weight * (0.5 - 0.5 * cos(scan_phase));
    color *= scanline;

    // 5. Aperture Grille / RGB Subpixel Phosphor Mask
    // Balatro uses a staggered RGB triad in virtual pixel space
    vec2 virtual_pixel = uv * vec2(virtual_rows * (pc.resolution.x / pc.resolution.y), virtual_rows);
    float subpixel = mod(floor(virtual_pixel.x * 3.0), 3.0);
    
    vec3 mask = vec3(1.0);
    if (subpixel < 1.0)      mask = vec3(1.0, 0.7, 0.7);
    else if (subpixel < 2.0) mask = vec3(0.7, 1.0, 0.7);
    else                     mask = vec3(0.7, 0.7, 1.0);
    
    color *= mix(vec3(1.0), mask, pc.mask_strength);

    // 6. Subtle VHS / RF Rolling Hum Bar
    float hum_bar = 1.0 + 0.025 * sin(uv.y * 8.0 + pc.time * 1.2);
    color *= hum_bar;

    // 7. Tube Vignette & Rounded Bezel Falloff
    float dist_sq = dot(center_dist, center_dist);
    float vignette = 1.0 - dist_sq * 0.85;
    float bezel = crt_bezel(uv, 0.012);

    color *= vignette * bezel * pc.bloom_boost;

    out_color = vec4(color, 1.0);
}