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
    float constant;
    float linear;
    float quadratic;
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

    // Check for invalid gbuffer data (background pixels or invalid positions)
    // If position is (0,0,0) or very close, skip lighting (this is background)
    if (length(FragPos) < 0.001) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Ensure minimum diffuse color to prevent pure black materials
    Diffuse = max(Diffuse, vec3(0.01));

    // Ambient lighting (matches forward shader)
    vec3 Ia = ambientColor * ambientStrength;
    vec3 result = Ia * Diffuse;

    for (int i = 0; i < min(numLights, 128); ++i)
    {
        // Calculate distance and attenuation
        vec3 lightDir = lights[i].position - FragPos;
        float distance = length(lightDir);
        
        // Prevent division by zero and ensure minimum distance
        distance = max(distance, 0.001);
        
        // Light culling: skip lights that are too far away to contribute meaningfully
        // Calculate max possible attenuation at this distance
        float maxAttenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        if (maxAttenuation < 0.01) continue; // Light too dim to matter (1% threshold)
        
        float attenuation = maxAttenuation;
        
        vec3 L = normalize(lightDir);
        vec3 V = normalize(viewPos - FragPos);
        vec3 R = reflect(-L, Normal);

        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(R, V), 0.0), Shininess);

        // Match forward shader exactly: Id * material.diffuse + Is * material.specular
        // where Id = lights[i].color * diff, Is = lights[i].color * spec
        vec3 Id = lights[i].color * diff;
        vec3 Is = lights[i].color * spec;
        
        // Apply attenuation to light contribution
        result += (Id * Diffuse + Is * Specular) * attenuation;
    }

    // Clamp to prevent negative values
    result = max(result, vec3(0.0));
    FragColor = vec4(result, 1.0);
}
