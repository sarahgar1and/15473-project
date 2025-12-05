#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex{
    glm::vec3 position;
    glm::vec3 normal;
};

class Mesh{
public:
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, size_t materialIndex = 0);
    void Draw() const;

    glm::mat4 transformation;
    size_t materialIndex;
    size_t triangleCount; // Number of triangles (indices.size() / 3)
    glm::vec3 center; // Bounding box center (for distance calculations)
    glm::vec3 bboxMin; // Bounding box minimum (local space)
    glm::vec3 bboxMax; // Bounding box maximum (local space)
    bool useForward = false; // Whether to use forward rendering for this mesh

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t vao, vbo, ebo;

};