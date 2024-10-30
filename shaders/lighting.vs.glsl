#version 410 core

layout(location = 0) in vec3 vPos;        // Vertex position
layout(location = 1) in vec3 vNormal;     // Vertex normal

// Uniforms
uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;

// Outputs to Fragment Shader
out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(mvpMatrix * vec4(vPos, 1.0));
    Normal = normalize(normalMatrix * vNormal);

    gl_Position = mvpMatrix * vec4(vPos, 1.0);
}
