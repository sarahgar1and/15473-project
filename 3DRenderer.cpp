#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>

int main(){

    const char* vertexShaderCode = "#version 330 core\n"
        "layout (location = 0) in vec3 pos;\n"
        "void main(){ gl_Position = vec4(pos, 1.0); }\n";
    
    const char* fragmentShaderCode = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main(){ FragColor = vec4(1.0, 1.0, 0.0, 1.0); } \n";

    // Request OpenGL 3.3 Core Profile
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::Window window(sf::VideoMode({800,800}), "3D OpenGL", sf::Style::Default, sf::State::Windowed, settings);

    // glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK){
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    int success{};
    char infoLog[1024]{};

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

    size_t shader = glCreateProgram();
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);

    glGetProgramiv(shader, GL_LINK_STATUS, &success);

    if (!success){
        glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Failed to link shader!\nInfolog:\n" << infoLog;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    unsigned int vao{}, vbo{};
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while(window.isOpen()){
        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

         window.display();

    }
}