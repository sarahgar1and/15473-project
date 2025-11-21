#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform float ambientStrength;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform struct {
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

void main(){ 
    // ambient 
    vec3 Ia = lightColor * ambientStrength;

    // diffuse
    vec3 N = normalize(Normal);
    vec3 Lm = normalize(lightPos - FragPos);
    vec3 Id = lightColor * max(dot(N, Lm), 0.0);

    // specular
    vec3 V = normalize(viewPos - FragPos);
    vec3 Rm = reflect(-Lm, N);
    vec3 Is = lightColor * pow(max(dot(Rm, V), 0.0), material.shininess) * 0.5;

    vec3 finalColor = (Ia + Id) * material.diffuse + Is * material.specular;

    FragColor = vec4(finalColor, 1.0); 
}