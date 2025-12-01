#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 viewPos;

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

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights; ++i)
    {
        vec3 L = normalize(lights[i].position - FragPos);
        vec3 V = normalize(viewPos - FragPos);
        vec3 R = reflect(-L, Normal);

        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(R, V), 0.0), Shininess);

        result += Diffuse * diff * lights[i].color + spec * lights[i].color;
    }

    FragColor = vec4(result, 1.0);
}
