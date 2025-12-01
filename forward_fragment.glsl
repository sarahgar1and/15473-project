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
    // ambient 
    vec3 Ia = ambientColor * ambientStrength;
    vec3 finalColor = Ia * material.diffuse;

    for (int i = 0; i < min(numLights, MAX_LIGHTS); i++){    
        // diffuse
        vec3 N = normalize(Normal);
        vec3 Lm = normalize(lights[i].position - FragPos);
        vec3 Id = lights[i].color * max(dot(N, Lm), 0.0);

        // specular
        vec3 V = normalize(viewPos - FragPos);
        vec3 Rm = reflect(-Lm, N);
        vec3 Is = lights[i].color * pow(max(dot(Rm, V), 0.0), material.shininess);

        finalColor += Id * material.diffuse + Is * material.specular;
    }

    FragColor = vec4(finalColor, material.opacity); 
}