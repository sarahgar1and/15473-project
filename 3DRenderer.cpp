#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Object.h"
#include "Model.h"

// const float speed = 6.0f;
// const float mouseSensitivity = 25.0f;

std::string ReadTextFile(const std::string& fileName);

int main(){

    // Request OpenGL 3.3 Core Profile
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::Window window(sf::VideoMode({800,800}), "3D OpenGL", sf::Style::Default, sf::State::Windowed, settings);

    if (glewInit() != GLEW_OK){
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    Shader shader(ReadTextFile("vertex.glsl"), ReadTextFile("fragment.glsl"));

    Mesh mesh({
        glm::vec3(0.8f, 0.8f, 0.0f), // top right
        glm::vec3(0.8f, -0.8f, 0.0f), // bottom right
        glm::vec3(-0.8f, -0.8f, 0.0f), // bottom left
        glm::vec3(-0.8f, 0.8f, 0.0f) // top left
    }, {0, 1, 3, 1, 2, 3});

    Model model("");

    Camera camera(glm::vec3(0.0f, 0.0f, 2.0f));
    Object object(&model, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(45.0f, 45.0f, 45.0f));

    // bool isFirstMouse = true;
    // sf::Vector2i lastMousePos{};

    sf::Clock clock{};
    while (window.isOpen()){
        float deltaTime = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (event->is<sf::Event::Resized>()){
                glViewport(0, 0, window.getSize().x, window.getSize().y);
            }
        }
        // Camera Movement
        camera.UpdateDirectionVectors();

        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        //     camera.position += camera.Forward() * speed * deltaTime;
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        //     camera.position -= camera.Forward() * speed * deltaTime;
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        //     camera.position -= camera.Right() * speed * deltaTime;
        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        //     camera.position += camera.Right() * speed * deltaTime; 

        // if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)){
        //     if (isFirstMouse){
        //         lastMousePos = sf::Mouse::getPosition(window);
        //         isFirstMouse = false;
        //         window.setMouseCursorVisible(false);
        //     } else {
        //         sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        //         int xOffset = mousePos.x - lastMousePos.x;
        //         int yOffset = lastMousePos.y - mousePos.y;

        //         camera.yaw += xOffset * mouseSensitivity * deltaTime;
        //         camera.pitch += yOffset * mouseSensitivity * deltaTime;

        //         sf::Mouse::setPosition(lastMousePos, window);
        //     }
        // } else {
        //     isFirstMouse = true;
        //     window.setMouseCursorVisible(true);
        // }

        // Rendering
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Use();
        shader.SetValue("view", camera.GetViewMatrix());
        shader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));

        object.Draw(shader, glm::vec3(1.0f, 0.5f, 0.5f));

        window.display();
    }
}

std::string ReadTextFile(const std::string& fileName){
    std::ifstream file(fileName);

    std::stringstream ss{};
    ss << file.rdbuf();
    file.close();

    return ss.str();
}