#pragma once

#include <glm/glm.hpp>
#include <vector>

class Mesh{
public:
    Mesh(std::vector<glm::vec3> vertices, std::vector<uint32_t> indices);
    void Draw() const;

private:
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;

    uint32_t vao, vbo, ebo;

};