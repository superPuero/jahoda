#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outAlbedo;

void main() {
    outAlbedo = vec4(fragColor, 1.0);
}