#version 330 core

layout(location = 0) in vec3 vPos; // Ensure this line exists

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = vPos;
    vec4 pos = projection * view * vec4(vPos, 1.0);
    gl_Position = pos.xyww; // This will push the skybox to the far depth
}
