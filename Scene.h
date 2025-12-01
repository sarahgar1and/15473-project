#pragma once

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"

#include <string>
#include <unordered_map>
#include <assimp/scene.h>

struct Material{
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    bool useForward = false;
};

struct Light{
    glm::vec3 position;
    glm::vec3 color;
};

class Scene{
public:
    Scene(const std::string& fileName);
    void DrawForward(Shader& shader) const; 
    void DrawDeferred(Shader& shader) const; 
    void SetLights(Shader& shader) const;
    Camera camera;

private:
    void processNode(aiNode* node, const aiScene* scene , glm::mat4 parentTransformation,
        std::unordered_map<std::string, glm::mat4>& nodeTransformations);
    Material processMaterials(aiMaterial* material);
    Mesh processMesh(aiMesh* mesh);

    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Light> lights;
};