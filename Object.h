#pragma once

#include "Mesh.h"
#include "Shader.h"

class Object{
public:
    Object(const Mesh* mesh, glm::vec3 position = {}, glm::vec3 rotation = {}, glm::vec3 scale = glm::vec3(1.0f));
    void Draw(Shader& shader, glm::vec3 color = {});

    glm::mat4 GetTransformationMatrx();

    const Mesh* mesh;
    glm::vec3 position, rotation, scale;

private:
};