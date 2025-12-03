#version 330 core

#define MAX_LIGHTS 128

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform float ambientStrength;
uniform vec3 ambientColor;
uniform vec3 viewPos;

struct Light {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform Light lights[MAX_LIGHTS];

uniform int numLights;

uniform struct {
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float opacity;
} material;

void main(){ 
    // Ensure minimum diffuse color to prevent pure black materials
    vec3 diffuse = max(material.diffuse, vec3(0.01));
    
    // ambient 
    vec3 Ia = ambientColor * ambientStrength;
    vec3 finalColor = Ia * diffuse;

    // Normalize normal once (matches deferred shader)
    vec3 N = normalize(Normal);

    for (int i = 0; i < min(numLights, MAX_LIGHTS); i++){    
        // Calculate distance and attenuation
        vec3 lightDir = lights[i].position - FragPos;
        float distance = length(lightDir);
        
        // Prevent division by zero and ensure minimum distance
        distance = max(distance, 0.001);
        
        float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
        
        // diffuse
        vec3 Lm = normalize(lightDir);
        vec3 Id = lights[i].color * max(dot(N, Lm), 0.0);

        // specular
        vec3 V = normalize(viewPos - FragPos);
        vec3 Rm = reflect(-Lm, N);
        vec3 Is = lights[i].color * pow(max(dot(Rm, V), 0.0), material.shininess);

        // Apply attenuation to light contribution
        finalColor += (Id * diffuse + Is * material.specular) * attenuation;
    }

    // Clamp to prevent negative values
    finalColor = max(finalColor, vec3(0.0));
    FragColor = vec4(finalColor, material.opacity); 
}