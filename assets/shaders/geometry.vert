#version 450

layout(location = 0) out vec3 fragColor;

// 1. Define the 8 corners of a unit cube (-0.5 to 0.5)
vec3 cubeVertices[8] = vec3[](
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5)
);

// 2. Define the 36 indices required to draw 12 triangles (6 faces)
int indices[36] = int[](
    0, 1, 2, 2, 3, 0, // Front
    1, 5, 6, 6, 2, 1, // Right
    7, 6, 5, 5, 4, 7, // Back
    4, 0, 3, 3, 7, 4, // Left
    4, 5, 1, 1, 0, 4, // Bottom
    3, 2, 6, 6, 7, 3  // Top
);

void main() {
    vec3 pos = cubeVertices[indices[gl_VertexIndex]];

    // 1. Better Rotation Angles
    float time = 0.5; // Manual offset for now
    mat3 rotX = mat3(1, 0, 0, 0, cos(time), -sin(time), 0, sin(time), cos(time));
    mat3 rotY = mat3(cos(time), 0, sin(time), 0, 1, 0, -sin(time), 0, cos(time));

    vec3 rotatedPos = rotY * rotX * pos;

    // 2. Fix Aspect Ratio (Assuming 16:9 window, adjust as needed)
    float aspect = 600.0 / 600.0;
    rotatedPos.x /= aspect; 

    // 3. Scale and Shift Depth
    // Scale by 0.5 so it fits in the -1 to 1 screen space easily
    // Shift Z by +0.5 so the cube sits in the 0.0 to 1.0 range
    gl_Position = vec4(rotatedPos.xy * 0.5, (rotatedPos.z * 0.5) + 0.5, 1.0);
    
    fragColor = pos + 0.5;
}