#version 410 core

uniform sampler2D textureMap;

in vec2 TexCoords; // Texture coordinates from the vertex shader
out vec4 fragColorOut; // Output color

void main() {
    fragColorOut = texture(textureMap, TexCoords);
}
