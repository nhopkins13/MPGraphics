#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 texCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Only apply rotation by zeroing out the translation part of view matrix
    mat4 viewRotOnly = mat4(mat3(view)); // Keeps rotation, discards translation

    vec4 pos = projection * viewRotOnly * vec4(aPos, 1.0f);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);

    // Flip z-axis for correct coordinate space
    texCoords = vec3(aPos.x, aPos.y, -aPos.z);
}
