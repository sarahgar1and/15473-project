#include "Object.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Object::Object(const Model* model, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) 
    : model(model), position(position), rotation(rotation), scale(scale){
    
}

void Object::Draw(Shader& shader){
    model->Draw(shader, GetTransformationMatrix());
}

glm::mat4 Object::GetTransformationMatrix(){
    glm::mat4 t = glm::mat4(1.0f);

    t = glm::translate(t, position);
    t = t * glm::mat4_cast(glm::quat(glm::radians(rotation)));
    t = glm::scale(t, scale);

    return t;
}