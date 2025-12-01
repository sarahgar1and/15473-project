#pragma once

#include <glm/glm.hpp>
#include <vector>

class Quad {
public:
    Quad();
    void Draw() const;
private:
    uint32_t vao, vbo, ebo;
};