#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Scene.h"

std::string ReadTextFile(const std::string& fileName);

int main(int argc, char** argv){

    // Request OpenGL 3.3 Core Profile
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.depthBits = 24;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::Window window(sf::VideoMode({800,800}), "3D OpenGL", sf::Style::Default, sf::State::Windowed, settings);

    if (glewInit() != GLEW_OK){
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    std::string fileName = "scene.fbx";
    if (argc > 1) fileName = argv[1]; 

    Shader shader(ReadTextFile("vertex.glsl"), ReadTextFile("fragment.glsl"));
    Scene scene(fileName);

    Camera camera = scene.camera;
    camera.UpdateDirectionVectors();

    shader.Use();
    shader.SetValue("ambientStrength", 0.1f);
    // shader.SetValue("lights[0].color", glm::vec3(1.0f));
    shader.SetValue("ambientColor", 1.0f);
    // shader.SetValue("numLights", 1);

    // sf::Clock clock{};
    while (window.isOpen()){
        // float deltaTime = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (event->is<sf::Event::Resized>()){
                glViewport(0, 0, window.getSize().x, window.getSize().y);
            }
        }

        // Rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();
        shader.SetValue("view", camera.GetViewMatrix());
        shader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));
        // shader.SetValue("lights[0].position", camera.position);
        shader.SetValue("viewPos", camera.position);

        scene.Draw(shader);

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