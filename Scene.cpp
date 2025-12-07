#include "Scene.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <GL/glew.h>

// Initialize static threshold members
float Scene::HIGH_OVERDRAW_THRESHOLD = 2.0f;
float Scene::LOW_OVERDRAW_THRESHOLD = 1.2f;
size_t Scene::SMALL_MESH_THRESHOLD = 50;
size_t Scene::FEW_LIGHTS_THRESHOLD = 8;


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
            
            glm::vec3 fbxColor = glm::vec3(col.r, col.g, col.b);
            float maxComp = std::max({fbxColor.r, fbxColor.g, fbxColor.b});
            if (maxComp > 1.0f) {
                myLight.color = fbxColor / maxComp;  // Normalize to [0, 1] preserving ratios
            } else {
                myLight.color = fbxColor;  // Already in [0, 1] range
            }
            // myLight.color = glm::vec3(1.0f);

            myLight.constant = light->mAttenuationConstant > 0.0f ? light->mAttenuationConstant : 1.0f;
            myLight.linear = light->mAttenuationLinear >= 0.0f ? light->mAttenuationLinear : 0.014f;
            myLight.quadratic = light->mAttenuationQuadratic >= 0.0f ? light->mAttenuationQuadratic : 0.0007f;
            
            // Calculate light radius based on attenuation
            // Radius is the distance at which light brightness drops to 5/256 (essentially invisible)
            float lightMax = std::max({myLight.color.r, myLight.color.g, myLight.color.b});
            if (myLight.quadratic > 0.0f && lightMax > 0.0f) {
                float discriminant = myLight.linear * myLight.linear - 4.0f * myLight.quadratic * (myLight.constant - (256.0f / 5.0f) * lightMax);
                if (discriminant >= 0.0f) {
                    myLight.radius = (-myLight.linear + std::sqrt(discriminant)) / (2.0f * myLight.quadratic);
                } else {
                    myLight.radius = 1000.0f; // Fallback: very large radius if calculation fails
                }
            } else {
                myLight.radius = 1000.0f; // Fallback: very large radius if quadratic is 0
            }

            lights.push_back(myLight);
        }
    }
        
    // Note: UpdateRenderingMode will be called after shaders are created
    // (called from main after gbufferShader is available)
}

void Scene::SetLights(Shader& shader) const {
    int i = 0;
    for (const auto& light : lights){
        shader.SetValue("lights[" + std::to_string(i) + "].position", light.position);
        shader.SetValue("lights[" + std::to_string(i) + "].color", light.color);
        shader.SetValue("lights[" + std::to_string(i) + "].constant", light.constant);
        shader.SetValue("lights[" + std::to_string(i) + "].linear", light.linear);
        shader.SetValue("lights[" + std::to_string(i) + "].quadratic", light.quadratic);
        shader.SetValue("lights[" + std::to_string(i) + "].radius", light.radius);
        i++;
    }
    shader.SetValue("numLights", i);
}

int Scene::DrawForward(Shader& shader) const {
    shader.Use();
    SetLights(shader);

    int count = 0;
    for (const auto& mesh : meshes){
        if (!mesh.useForward) 
            continue; // SKIP deferred meshes

        const Material& material = materials[mesh.materialIndex];

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
        if (mesh.useForward)
            continue; // SKIP forward-only meshes

        const Material& material = materials[mesh.materialIndex];

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

    // Default values in case material properties are missing
    material.diffuse = glm::vec3(0.8f, 0.8f, 0.8f); // Default gray
    material.specular = glm::vec3(0.5f, 0.5f, 0.5f); // Default gray specular

    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, col) == AI_SUCCESS) {
    material.diffuse = glm::vec3(col.r, col.g, col.b);
    }

    if (mat->Get(AI_MATKEY_COLOR_SPECULAR, col) == AI_SUCCESS) {
        material.specular = glm::vec3(col.r, col.g, col.b);
    }

    if (mat->Get(AI_MATKEY_SHININESS, material.shininess) != AI_SUCCESS) {
        material.shininess = 32.0f; // Default if not found
    }

    // Check for transparency/opacity
    if (mat->Get(AI_MATKEY_OPACITY, material.opacity) != AI_SUCCESS) {
        material.opacity = 1.0f; // Default if not found
    }

    return material;
}

void Scene::UpdateRenderingMode(Shader& gbufferShader, int viewportWidth, int viewportHeight, Mode mode) {
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix((float)viewportWidth, (float)viewportHeight);
    
    for (size_t i = 0; i < meshes.size(); i++) {
        const Mesh& mesh = meshes[i];        

        if (mode == DEFERRED){
            meshes[i].useForward = false; 
            continue;
        } else if (mode == FORWARD){
            meshes[i].useForward = true;
            continue;
        }
        
        // Hybrid Otherwise
        const Material& material = materials[mesh.materialIndex];

        // Transform mesh center to world space
        glm::vec3 worldCenter = glm::vec3(mesh.transformation * glm::vec4(mesh.center, 1.0f));
        
        float overdrawRatio = MeasureOverdraw(mesh, gbufferShader, viewportWidth, viewportHeight, view, projection);
        
        // Use heuristic to determine rendering mode
        meshes[i].useForward = ShouldUseForward(
            material, 
            mesh.triangleCount,
            lights.size(),
            overdrawRatio
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
    result.bboxMin = minPos; // Store bounding box min
    result.bboxMax = maxPos; // Store bounding box max
    return result;
}

float Scene::MeasureOverdraw(const Mesh& mesh, Shader& shader, int viewportWidth, int viewportHeight,
                              const glm::mat4& view, const glm::mat4& projection) const {

    // Calculate bounding box in screen space
    glm::mat4 mvp = projection * view * mesh.transformation;

    // Get all 8 corners of the bounding box
    glm::vec3 corners[8] = {
        glm::vec3(mesh.bboxMin.x, mesh.bboxMin.y, mesh.bboxMin.z),
        glm::vec3(mesh.bboxMax.x, mesh.bboxMin.y, mesh.bboxMin.z),
        glm::vec3(mesh.bboxMin.x, mesh.bboxMax.y, mesh.bboxMin.z),
        glm::vec3(mesh.bboxMax.x, mesh.bboxMax.y, mesh.bboxMin.z),
        glm::vec3(mesh.bboxMin.x, mesh.bboxMin.y, mesh.bboxMax.z),
        glm::vec3(mesh.bboxMax.x, mesh.bboxMin.y, mesh.bboxMax.z),
        glm::vec3(mesh.bboxMin.x, mesh.bboxMax.y, mesh.bboxMax.z),
        glm::vec3(mesh.bboxMax.x, mesh.bboxMax.y, mesh.bboxMax.z)
    };

    // Transform to screen space and find bounding rectangle
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    bool anyVisible = false;

    for (int i = 0; i < 8; i++) {
        glm::vec4 clipPos = mvp * glm::vec4(corners[i], 1.0f);

        // Perspective divide
        if (clipPos.w > 0.0f) {
            float x = clipPos.x / clipPos.w;
            float y = clipPos.y / clipPos.w;

            // Convert from NDC [-1,1] to screen coordinates [0, viewportWidth/Height]
            x = (x + 1.0f) * 0.5f * viewportWidth;
            y = (1.0f - y) * 0.5f * viewportHeight; // Flip Y

            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
            anyVisible = true;
        }
    }

    // If mesh is not visible, return default
    if (!anyVisible) return 1.0f;

    // Clamp to viewport bounds
    int bboxMinX = std::max(0, (int)std::floor(minX));
    int bboxMaxX = std::min(viewportWidth - 1, (int)std::ceil(maxX));
    int bboxMinY = std::max(0, (int)std::floor(minY));
    int bboxMaxY = std::min(viewportHeight - 1, (int)std::ceil(maxY));

    int bboxWidth = bboxMaxX - bboxMinX + 1;
    int bboxHeight = bboxMaxY - bboxMinY + 1;

    // If bounding box is invalid, return default
    if (bboxWidth <= 0 || bboxHeight <= 0) return 1.0f;

    // Create a temporary framebuffer with stencil buffer
    GLuint fbo, colorTex, depthStencilRB;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &colorTex);
    glGenRenderbuffers(1, &depthStencilRB);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Color attachment (needed for rendering)
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    // Depth + Stencil renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewportWidth, viewportHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRB);

    // Set viewport to match framebuffer size
    glViewport(0, 0, viewportWidth, viewportHeight);

    // Clear stencil to 0
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Disable color writes - we only care about stencil
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask); // Save current state
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable all color writes

    // Enable stencil testing to increment on depth pass
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // Increment stencil on depth pass
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);

    // Render only this mesh
    shader.Use();
    shader.SetValue("view", view);
    shader.SetValue("projection", projection);
    const Material& material = materials[mesh.materialIndex];
    shader.SetValue("model", mesh.transformation);
    shader.SetValue("material.diffuse", material.diffuse);
    shader.SetValue("material.specular", material.specular);
    shader.SetValue("material.shininess", material.shininess);
    mesh.Draw();

    // Re-enable color writes
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);

    // Read back stencil buffer only for the bounding box region
    std::vector<GLubyte> stencilData(bboxWidth * bboxHeight);
    glReadPixels(bboxMinX, bboxMinY, bboxWidth, bboxHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilData.data());

    // Calculate statistics
    unsigned long long totalFragments = 0;
    unsigned long long visiblePixels = 0;

    for (int i = 0; i < bboxWidth * bboxHeight; i++) {
        GLubyte stencilValue = stencilData[i];
        if (stencilValue > 0) {
            visiblePixels++;
            totalFragments += stencilValue;
        }
    }

    // Cleanup
    glDisable(GL_STENCIL_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &colorTex);
    glDeleteRenderbuffers(1, &depthStencilRB);

    // Return overdraw ratio (1.0 = no overdraw, 2.0 = 2x overdraw, etc.)
    if (visiblePixels == 0) return 1.0f;
    return (float)totalFragments / (float)visiblePixels;
}

bool Scene::ShouldUseForward(const Material& material, size_t triangleCount,
                             size_t numLights,
                             float overdrawRatio) {
    // 1. TRANSPARENCY: Must use forward for transparent objects
    // This is the most important check - deferred can't handle transparency properly
    if (material.opacity < 1.0f) {
        std::cout << "Transparent Object --> Forward" << std::endl;
        return true;
    }
    
    // 2. SMALL MESHES: Very small meshes are better in forward
    // Writing to multiple render targets has overhead, so small meshes benefit from forward
    if (triangleCount < SMALL_MESH_THRESHOLD) {
        std::cout << "Small Mesh: " << triangleCount <<" triangles --> Forward" << std::endl;
        return true;
    }
    
    // 3. FEW LIGHTS + LOW SCREEN COVERAGE: If very few lights and mesh covers little screen space, forward is better
    // Deferred shines with many lights or high screen coverage (G-buffer cost amortized)
    // if (numLights <= FEW_LIGHTS_THRESHOLD && triangleCount < SMALL_MESH_THRESHOLD) {
    //     std::cout << "Few Lights: " << numLights << " lights and Low Triangle Count: " << triangleCount << " triangles --> Forward" << std::endl;
    //     return true;
    // }
    
    // 4. OVERDRAW: High overdraw favors deferred (lighting done once per pixel)
    // Low overdraw with few lights favors forward (less overhead)
    if (overdrawRatio >= HIGH_OVERDRAW_THRESHOLD && numLights >= 3) {
        // High overdraw + many lights: deferred is better (lighting once per pixel)
        std::cout << "High overdraw: " << overdrawRatio <<"x and Many Lights: " << numLights << " --> Deferred" << std::endl;
        return false;
    }
    
    if (overdrawRatio < LOW_OVERDRAW_THRESHOLD && numLights <= FEW_LIGHTS_THRESHOLD) {
        // Low overdraw + few lights: forward is better (less overhead)
        std::cout << "Low overdraw: " << overdrawRatio <<"x and Few Lights: " << numLights << " --> Forward" << std::endl;
        return true;
    }
    
    // Default to deferred for opaque, medium-to-large meshes with multiple lights or high screen coverage
    return false;
}