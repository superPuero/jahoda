#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require 

layout(location = 0) in vec3 in_pos;

layout(buffer_reference, scalar, buffer_reference_align = 16) readonly buffer InstanceBuffer {
    mat4 transform[]; 
};
layout(buffer_reference, scalar) readonly buffer VisibilityIdBuffer { uint ids[]; };

layout(push_constant) uniform PushConstants {
    mat4 view_proj; 
    InstanceBuffer instance_buffer;
    VisibilityIdBuffer visibility_id_buffer;
} pc;

void main() 
{
    mat4 model = pc.instance_buffer.transform[pc.visibility_id_buffer.ids[gl_InstanceIndex]];
    gl_Position = pc.view_proj * model * vec4(in_pos, 1.0);
}