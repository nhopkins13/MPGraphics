#version 410 core

// Vertex Attributes
layout(location = 0) in vec3 vPos;      // Vertex position
layout(location = 1) in vec3 vNormal;   // Vertex normal

// Uniforms
uniform mat4 mvpMatrix;                 // Model-View-Projection matrix
uniform mat3 normalMatrix;              // Normal matrix

// Outputs to Fragment Shader
out vec3 FragPos;                       // Fragment position in world space
out vec3 Normal;                        // Normal vector in world space

void main()
{
    // Transform vertex position to world space
    FragPos = vec3(mvpMatrix * vec4(vPos, 1.0));

    // Transform and normalize the normal vector
    Normal = normalize(normalMatrix * vNormal);

    // Final vertex position in clip space
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
}
