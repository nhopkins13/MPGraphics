#version 410 core

// Structs
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

// Uniforms
uniform Material material;
uniform vec3 lightDirection;    // Directional light direction
uniform vec3 lightColor;        // Light color
uniform sampler2D groundTexture; // Texture uniform

// Inputs from Vertex Shader
in vec3 FragPos;                // Fragment position in world space
in vec3 Normal;                 // Normal vector in world space
in vec2 TexCoords;

// Output
out vec4 FragColor;             // Final fragment color

void main()
{
    vec3 texColor = texture(groundTexture, TexCoords).rgb;
    FragColor = vec4(texColor, 1.0); // Use the texture color as output

    // Normalize the input normal vector
    vec3 norm = normalize(Normal);

    // Normalize the light direction
    vec3 lightDir = normalize(-lightDirection);

    // Ambient component
    vec3 ambient = material.ambient * lightColor;

    // Diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * lightColor;

    // Specular component
    vec3 viewDir = normalize(-FragPos); // Assuming the camera is at the origin
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * lightColor;

    // Combine all components
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
