#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <iostream>

Shader::Shader(const std::string& vertexCode, const std::string& fragmentCode){
    int success{};
    char infoLog[1024]{};

    const char* vertexShaderCode = vertexCode.c_str();
    const char* fragmentShaderCode = fragmentCode.c_str();

    size_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, nullptr);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success){
        glGetShaderInfoLog(vertexShader, 1024, nullptr, infoLog);
        std::cerr << "Failed to complie vertex shader!\nInfolog:\n" << infoLog;
    }

    size_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success){
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, infoLog);
        std::cerr << "Failed to complie fragment shader!\nInfolog:\n" << infoLog;
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    glGetProgramiv(programID, GL_LINK_STATUS, &success);

    if (!success){
        glGetProgramInfoLog(programID, 1024, nullptr, infoLog);
        std::cerr << "Failed to link shader!\nInfolog:\n" << infoLog;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::Use(){
    glUseProgram(programID);
}

void Shader::SetValue(const std::string& name, glm::vec3 value){
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location >= 0) {
        glUniform3f(location, value.x, value.y, value.z);
    } else {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader\n";
    }
}

void Shader::SetValue(const std::string& name, glm::mat4 value){
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location >= 0) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void Shader::SetValue(const std::string& name, float value){
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void Shader::SetValue(const std::string& name, int value){
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location >= 0) {
        glUniform1i(location, value);
    }
}