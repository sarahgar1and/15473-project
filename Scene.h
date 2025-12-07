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
    float constant = 1.0f;    // Constant attenuation
    float linear = 0.014f;   // Linear attenuation (weaker for larger scenes)
    float quadratic = 0.0007f; // Quadratic attenuation (weaker for larger scenes)
    float radius = 0.0f;      // Light volume radius (calculated from attenuation)
};

enum Mode{
    DEFERRED,
    FORWARD, 
    HYBRID
};

class Scene{
public:
    Scene(const std::string& fileName);
    int DrawForward(Shader& shader) const; // Returns number of objects rendered
    int DrawDeferred(Shader& shader) const; // Returns number of objects rendered 
    void SetLights(Shader& shader) const;
    size_t GetLightCount() const { return lights.size(); }
    
    // Global thresholds for rendering heuristics
    static float HIGH_OVERDRAW_THRESHOLD;
    static float LOW_OVERDRAW_THRESHOLD;
    static size_t SMALL_MESH_THRESHOLD;
    static float FAR_DISTANCE_THRESHOLD;
    static size_t FEW_LIGHTS_THRESHOLD;
    static size_t LARGE_MESH_THRESHOLD;
    static float LOW_SCREEN_COVERAGE_THRESHOLD; // Screen coverage threshold (0.0-1.0)

    // Heuristic function to determine if object should use forward rendering
    static bool ShouldUseForward(const Material& material, size_t triangleCount, 
                                 size_t numLights = 0,
                                 float overdrawRatio = 1.0f,
                                 float screenCoverage = 0.0f);
    
    // Structure to hold mesh metrics
    struct MeshMetrics {
        float overdrawRatio;    // totalFragments / visiblePixels (1.0 = no overdraw, 2.0 = 2x overdraw, etc.)
        float screenCoverage;   // Fraction of screen covered (0.0-1.0)
    };
    
    // Measure overdraw and screen coverage for a specific mesh (returns both metrics)
    MeshMetrics GetMetrics(const Mesh& mesh, Shader& shader, int viewportWidth, int viewportHeight,
                                 const glm::mat4& view, const glm::mat4& projection) const;
    
    // Update rendering mode for all meshes based on heuristics
    // Call this after scene is loaded or when camera/lighting changes
    void UpdateRenderingMode(Shader& gbufferShader, int viewportWidth, int viewportHeight, Mode mode);
    
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