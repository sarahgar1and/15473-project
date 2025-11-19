#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <assimp/scene.h>

class Model{
public:
    Model(const std::string& fileName);
    void Draw(Shader& shader, glm::mat4 transformation) const; 

private:
    void processNode(aiNode* node, glm::mat4 parentTransformation);

    std::vector<Mesh> meshes;
};