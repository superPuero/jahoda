#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

layout(push_constant) uniform PushConstants {
    mat4 projection;
    vec3 text_color;
	float scale;
	vec2 pos;
	vec2 padding;
} pc;

void main() 	
{
    out_uv = in_uv;
    gl_Position = pc.projection * vec4(pc.pos + (in_pos * pc.scale), 0.0, 1.0);
}