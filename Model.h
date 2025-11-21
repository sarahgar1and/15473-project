#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <assimp/scene.h>

struct Material{
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

class Model{
public:
    Model(const std::string& fileName);
    void Draw(Shader& shader, glm::mat4 transformation) const; 

private:
    void processNode(aiNode* node, const aiScene* scene , glm::mat4 parentTransformation);
    Material processMaterials(aiMaterial* material);
    Mesh processMesh(aiMesh* mesh);

    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};