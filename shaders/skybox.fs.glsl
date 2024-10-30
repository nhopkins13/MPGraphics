#version 330 core

in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox; // Ensure this is samplerCube

void main() {
    FragColor = texture(skybox, TexCoords);
}
