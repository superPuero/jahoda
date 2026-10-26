#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform texture2D in_albedo;
layout(set = 0, binding = 1) uniform texture2D in_normal;
layout(set = 0, binding = 2) uniform texture2D in_depth;

layout(location = 0) out vec4 outFinalColor;

void main()     
{
    vec4 albedo = texelFetch(in_albedo, ivec2(gl_FragCoord.xy), 0);
    vec3 normal = texelFetch(in_normal, ivec2(gl_FragCoord.xy), 0).xyz;
    float depth = texelFetch(in_depth, ivec2(gl_FragCoord.xy), 0).x;

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 lightColor = vec3(1.0, 0.9, 0.8);
    float ambient = 0.1;

    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 litColor = albedo.rgb * lightColor * diffuseFactor;
    
    litColor += albedo.rgb * ambient;
    
    outFinalColor = vec4(litColor, 1.0);
}