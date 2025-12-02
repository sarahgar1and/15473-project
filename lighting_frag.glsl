#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gSpecular;

uniform vec3 viewPos;
uniform float ambientStrength;
uniform vec3 ambientColor;

struct Light {
    vec3 position;
    vec3 color;
};
uniform Light lights[128];
uniform int numLights;

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal  = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Shininess = texture(gAlbedoSpec, TexCoords).a;
    vec3 Specular = texture(gSpecular, TexCoords).rgb;

    // Ambient lighting (matches forward shader)
    vec3 Ia = ambientColor * ambientStrength;
    vec3 result = Ia * Diffuse;

    for (int i = 0; i < min(numLights, 128); ++i)
    {
        vec3 L = normalize(lights[i].position - FragPos);
        vec3 V = normalize(viewPos - FragPos);
        vec3 R = reflect(-L, Normal);

        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(R, V), 0.0), Shininess);

        // Match forward shader exactly: Id * material.diffuse + Is * material.specular
        // where Id = lights[i].color * diff, Is = lights[i].color * spec
        vec3 Id = lights[i].color * diff;
        vec3 Is = lights[i].color * spec;
        result += Id * Diffuse + Is * Specular;
    }

    FragColor = vec4(result, 1.0);
}
