#include "Scene.h"

#include <iostream>
#include <algorithm>
#include <cfloat>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

Scene::Scene(const std::string& fileName) 
    : camera(glm::vec3(0.0f, 0.0f, 10.0f)) {
    Assimp::Importer importer;
    importer.ReadFile(fileName, aiProcess_Triangulate);

    const aiScene* scene = importer.GetScene();

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)){
        std::cerr << "Failed to load " << fileName << std::endl;
        return;
    }

    std::unordered_map<std::string, glm::mat4> nodeTransformations;
    processNode(scene->mRootNode, scene, glm::mat4(1.0f), nodeTransformations);

    for (size_t i = 0; i < scene->mNumMaterials; i++){
        materials.push_back(processMaterials(scene->mMaterials[i]));
    }

    // Light list
    for (size_t i = 0; i < scene->mNumLights; i++){
        aiLight* light = scene->mLights[i];
        if (light->mType == aiLightSource_POINT){
            Light myLight{};
            aiVector3D pos = light->mPosition;
            aiColor3D col = light->mColorDiffuse;

            myLight.position = glm::vec3(nodeTransformations[light->mName.data] * glm::vec4(pos.x, pos.y, pos.z, 1.0f));
            myLight.color = glm::vec3(col.r, col.g, col.b);
            myLight.color = glm::vec3(1.0f);

            lights.push_back(myLight);
        }
    }
    
    // Apply heuristics to determine forward vs deferred rendering
    UpdateRenderingMode();
}

void Scene::SetLights(Shader& shader) const {
    int i = 0;
    for (const auto& light : lights){
        shader.SetValue("lights[" + std::to_string(i) + "].position", light.position);
        shader.SetValue("lights[" + std::to_string(i) + "].color", light.color);
        i++;
    }
    shader.SetValue("numLights", i);
}

int Scene::DrawForward(Shader& shader) const {
    shader.Use();
    SetLights(shader);

    int count = 0;
    for (const auto& mesh : meshes){
        const Material& material = materials[mesh.materialIndex];

        if (!material.useForward) 
            continue; // SKIP deferred meshes

        shader.SetValue("model", mesh.transformation);
        shader.SetValue("material.diffuse", material.diffuse);
        shader.SetValue("material.specular", material.specular);
        shader.SetValue("material.shininess", material.shininess);
        shader.SetValue("material.opacity", material.opacity);

        mesh.Draw();
        count++;
    }
    return count;
}

int Scene::DrawDeferred(Shader& gbufferShader) const {
    gbufferShader.Use();

    int count = 0;
    for (const auto& mesh : meshes) {
        const Material& material = materials[mesh.materialIndex];

        if (material.useForward)
            continue; // SKIP forward-only meshes

        gbufferShader.SetValue("model", mesh.transformation);
        gbufferShader.SetValue("material.diffuse", material.diffuse);
        gbufferShader.SetValue("material.specular", material.specular);
        gbufferShader.SetValue("material.shininess", material.shininess);

        mesh.Draw();
        count++;
    }
    return count;
}


void Scene::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransformation,
    std::unordered_map<std::string, glm::mat4>& nodeTransformations){
    glm::mat4 transformation{};

    transformation[0][0] = node->mTransformation.a1;  transformation[1][0] = node->mTransformation.a2;
    transformation[2][0] = node->mTransformation.a3;  transformation[3][0] = node->mTransformation.a4;
    transformation[0][1] = node->mTransformation.b1;  transformation[1][1] = node->mTransformation.b2;
    transformation[2][1] = node->mTransformation.b3;  transformation[3][1] = node->mTransformation.b4;
    transformation[0][2] = node->mTransformation.c1;  transformation[1][2] = node->mTransformation.c2;
    transformation[2][2] = node->mTransformation.c3;  transformation[3][2] = node->mTransformation.c4;
    transformation[0][3] = node->mTransformation.d1;  transformation[1][3] = node->mTransformation.d2;
    transformation[2][3] = node->mTransformation.d3;  transformation[3][3] = node->mTransformation.d4;

    transformation = parentTransformation * transformation;
    nodeTransformations[node->mName.data] = transformation;

    for (size_t i = 0; i < node->mNumMeshes; i++){
        Mesh mesh = processMesh(scene->mMeshes[node->mMeshes[i]]);
        mesh.transformation = transformation;
        meshes.push_back(mesh);
    }

    for (size_t i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene, transformation, nodeTransformations);
    }
}

Material Scene::processMaterials(aiMaterial* mat){
    Material material{};
    aiColor3D col;

    mat->Get(AI_MATKEY_COLOR_DIFFUSE, col);
    material.diffuse = glm::vec3(col.r, col.g, col.b);

    mat->Get(AI_MATKEY_COLOR_SPECULAR, col);
    material.specular = glm::vec3(col.r, col.g, col.b);

    mat->Get(AI_MATKEY_SHININESS, material.shininess);
    
    // Check for transparency/opacity
    float opacity = 1.0f;
    if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        material.opacity = opacity;
    }

    return material;
}

void Scene::UpdateRenderingMode() {
    // Update useForward flag for each material based on heuristics
    // This should be called after all meshes and materials are loaded
    for (size_t i = 0; i < meshes.size(); i++) {
        const Mesh& mesh = meshes[i];
        const Material& material = materials[mesh.materialIndex];
        
        // Transform mesh center to world space
        glm::vec3 worldCenter = glm::vec3(mesh.transformation * glm::vec4(mesh.center, 1.0f));
        
        // Use heuristic to determine rendering mode
        materials[mesh.materialIndex].useForward = ShouldUseForward(
            material, 
            mesh.triangleCount,
            worldCenter,
            camera.position,
            lights.size()
        );
    }
}

Mesh Scene::processMesh(aiMesh* mesh){
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Compute bounding box center for distance calculations
    glm::vec3 minPos(FLT_MAX), maxPos(-FLT_MAX);
    
    for (size_t i = 0; i < mesh->mNumVertices; i++){
        Vertex vertex{};
        aiVector3D pos = mesh->mVertices[i];
        aiVector3D norm = mesh->mNormals[i];
        vertex.position = glm::vec3(pos.x, pos.y, pos.z);
        vertex.normal = glm::vec3(norm.x, norm.y, norm.z);

        // Update bounding box
        minPos.x = std::min(minPos.x, vertex.position.x);
        minPos.y = std::min(minPos.y, vertex.position.y);
        minPos.z = std::min(minPos.z, vertex.position.z);
        maxPos.x = std::max(maxPos.x, vertex.position.x);
        maxPos.y = std::max(maxPos.y, vertex.position.y);
        maxPos.z = std::max(maxPos.z, vertex.position.z);

        vertices.push_back(vertex);
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        for (size_t i = 0; i < face.mNumIndices; i++){
            indices.push_back(face.mIndices[i]);
        }
    }

    Mesh result(vertices, indices, mesh->mMaterialIndex);
    result.center = (minPos + maxPos) * 0.5f; // Bounding box center
    return result;
}

// Heuristic function to determine forward vs deferred rendering
bool Scene::ShouldUseForward(const Material& material, size_t triangleCount,
                             const glm::vec3& meshCenter,
                             const glm::vec3& cameraPos,
                             size_t numLights) {
    // 1. TRANSPARENCY: Must use forward for transparent objects
    // This is the most important check - deferred can't handle transparency properly
    if (material.opacity < 0.99f) {
        return true;
    }
    
    // 2. SMALL MESHES: Very small meshes are better in forward
    // Writing to multiple render targets has overhead, so small meshes benefit from forward
    const size_t SMALL_MESH_THRESHOLD = 50; // triangles
    if (triangleCount < SMALL_MESH_THRESHOLD) {
        return true;
    }
    
    // 3. DISTANCE: Far objects might be better in forward
    // Far objects are affected by fewer lights and have less detail
    float distance = glm::length(meshCenter - cameraPos);
    const float FAR_DISTANCE_THRESHOLD = 100.0f;
    if (distance > FAR_DISTANCE_THRESHOLD) {
        return true;
    }
    
    // 4. FEW LIGHTS: If very few lights, forward might be simpler
    // Deferred shines with many lights, forward is fine for few lights
    const size_t FEW_LIGHTS_THRESHOLD = 2;
    if (numLights <= FEW_LIGHTS_THRESHOLD && triangleCount < 500) {
        return true;
    }
    
    // 5. VERY HIGH SHININESS: Highly reflective materials might benefit from deferred
    // (This is a case where deferred is better, so we return false)
    // But if combined with other factors, might still use forward
    
    // Default to deferred for opaque, medium-to-large meshes with multiple lights
    return false;
}