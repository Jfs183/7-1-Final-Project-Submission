#version 330 core

// Vertex attributes
layout(location = 0) in vec3 aPos;       // position
layout(location = 1) in vec3 aNormal;    // normal
layout(location = 2) in vec2 aTexCoord;  // texture coordinate

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    // Transform vertex position to world space
    vec4 worldPosition = model * vec4(aPos, 1.0);
    FragPos = worldPosition.xyz;

    // Transform normal vector
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass through texture coordinates
    TexCoord = aTexCoord;

    // Final vertex position in clip space
    gl_Position = projection * view * worldPosition;
}
