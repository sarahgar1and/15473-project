#version 330 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec; // rgb = diffuse, a = shininess

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} fs_in;

uniform struct {
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

void main()
{
    gPosition = fs_in.FragPos;
    gNormal   = normalize(fs_in.Normal);
    gAlbedoSpec.rgb = material.diffuse;
    gAlbedoSpec.a   = material.shininess; // optionally encode specular intensity here
}
