#version 450

layout(binding = 0) uniform sampler2D font_atlas;

layout(push_constant) uniform TextParams {
    mat4 projection;
    vec3 text_color; 
	float scale;	
	vec2 pos;
	vec2 padding;
} params;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() 
{
    float text_alpha = texture(font_atlas, in_uv).r;
    out_color = vec4(params.text_color, text_alpha);
}