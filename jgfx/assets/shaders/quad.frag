#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(set = 0, binding = 0) uniform texture2D in_albedo;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = texelFetch(in_albedo, ivec2(gl_FragCoord.xy), 0);
}
