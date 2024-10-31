#version 330 core
out vec4 FragColor;

in vec3 texCoords;

uniform samplerCube skybox;

void main()
{
    // Sample the color from the cubemap
    FragColor = texture(skybox, texCoords);
}
