#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex{
    glm::vec3 position;
    glm::vec3 normal;
};

class Mesh{
public:
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    void Draw() const;

    glm::mat4 transformation;

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t vao, vbo, ebo;

};