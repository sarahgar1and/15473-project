#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera{
public:
    static const glm::vec3 WorldUp;

    // @param position Camera (x,y,z) 
    // @param yaw Euler Angle, 90deg = forward direction
    // @param pitch Euler Angle
    // @param fov Field of View
    Camera(glm::vec3 position, float yaw = -90.0f, float pitch = 0.0f, float fov = 60.0f);

    void UpdateDirectionVectors();
    glm::vec3 Forward();
    glm::vec3 Right();
    glm::vec3 Up();
    glm::mat4 GetProjectionMatrix(float width, float height);
    glm::mat4 GetViewMatrix();

    glm::vec3 position;
    float fov, yaw, pitch; // In degrees
private:
    glm::vec3 forward, right, up;
};