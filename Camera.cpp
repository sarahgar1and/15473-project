#include "Camera.h"

const glm::vec3 Camera::WorldUp = {0.0f, 1.0f, 0.0f};

Camera::Camera(glm::vec3 position, float yaw, float pitch, float fov)
    : position(position), yaw(yaw), pitch(pitch), fov(fov), forward(), right(), up()
{    
}

glm::mat4 Camera::GetProjectionMatrix(float width, float height){
    return glm::perspective(glm::radians(fov), width / height, 0.1f, 100.0f);
}

void Camera::UpdateDirectionVectors(){
    forward = glm::normalize(glm::vec3(
        glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
        glm::sin(glm::radians(pitch)),
        glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
    ));
    right = glm::normalize(glm::cross(forward, WorldUp));
    up = glm::normalize(glm::cross(right, forward));
}

glm::mat4 Camera::GetViewMatrix(){
    return glm::lookAt(position, position + forward, up);
}

glm::vec3 Camera::Forward(){ return forward; }
glm::vec3 Camera::Right(){ return right; }
glm::vec3 Camera::Up(){ return up; }