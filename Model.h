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
    void processNode(aiNode* node, const aiScene* scene , glm::mat4 parentTransformation);
    Mesh processMesh(aiMesh* mesh);

    std::vector<Mesh> meshes;
};