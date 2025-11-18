#include "Object.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Object::Object(const Mesh* mesh, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) 
    : mesh(mesh), position(position), rotation(rotation), scale(scale){
    
}

void Object::Draw(Shader& shader, glm::vec3 color){
    shader.SetValue("model", GetTransformationMatrx());
    shader.SetValue("color", color);
    mesh->Draw();
}

glm::mat4 Object::GetTransformationMatrx(){
    glm::mat4 t = glm::mat4(1.0f);

    t = glm::translate(t, position);
    t = t * glm::mat4_cast(glm::quat(glm::radians(rotation)));
    t = glm::scale(t, scale);

    return t;
}