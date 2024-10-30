#version 410 core

layout(location = 0) in vec3 vPos;        // Vertex position
layout(location = 2) in vec2 textureCoords; // Vertex texture coordinates

uniform mat4 mvpMatrix;

// Outputs to Fragment Shader
out vec2 TexCoords;

void main() {
    TexCoords = textureCoords; // Pass texture coordinates to fragment shader
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
}
