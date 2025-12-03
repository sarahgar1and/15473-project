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
    float opacity = 1.0f; // 1.0 = opaque, < 1.0 = transparent
};

struct Light{
    glm::vec3 position;
    glm::vec3 color;
};

class Scene{
public:
    Scene(const std::string& fileName);
    int DrawForward(Shader& shader) const; // Returns number of objects rendered
    int DrawDeferred(Shader& shader) const; // Returns number of objects rendered 
    void SetLights(Shader& shader) const;
    
    // Heuristic function to determine if object should use forward rendering
    static bool ShouldUseForward(const Material& material, size_t triangleCount, 
                                 const glm::vec3& meshCenter = glm::vec3(0.0f),
                                 const glm::vec3& cameraPos = glm::vec3(0.0f),
                                 size_t numLights = 0);
    
    // Update rendering mode for all meshes based on heuristics
    // Call this after scene is loaded or when camera/lighting changes
    void UpdateRenderingMode();
    
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