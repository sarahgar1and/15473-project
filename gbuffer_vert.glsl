#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} vs_out;


void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;

    // Store world-space normal (just like forward pass)
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * worldPos;
}
