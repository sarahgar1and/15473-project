#include "Model.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

Model::Model(const std::string& fileName){
    Assimp::Importer importer;
    importer.ReadFile(fileName, aiProcess_Triangulate);

    const aiScene* scene = importer.GetScene();

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE){
        std::cerr << "Failed to load " << fileName << std::endl;
        return;
    }
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
}

void Model::Draw(Shader& shader, glm::mat4 transformation) const {
    for (const auto& mesh : meshes){
        shader.Use();
        shader.SetValue("model", mesh.transformation * transformation);
        mesh.Draw();
    }
}

void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransformation){
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

    for (size_t i = 0; i < node->mNumMeshes; i++){
        Mesh mesh = processMesh(scene->mMeshes[node->mMeshes[i]]);
        mesh.transformation = transformation;
        meshes.push_back(mesh);
    }

    for (size_t i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene, transformation);
    }
}

Mesh Model::processMesh(aiMesh* mesh){
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (size_t i = 0; i < mesh->mNumVertices; i++){
        Vertex vertex{};
        aiVector3D pos = mesh->mVertices[i];
        aiVector3D norm = mesh->mNormals[i];
        vertex.position = glm::vec3(pos.x, pos.y, pos.z);
        vertex.normal = glm::vec3(norm.x, norm.y, norm.z);;

        vertices.push_back(vertex);
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        for (size_t i = 0; i < face.mNumIndices; i++){
            indices.push_back(face.mIndices[i]);
        }
    }

    return Mesh(vertices, indices);
}