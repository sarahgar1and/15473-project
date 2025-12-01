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
#include "GBuffer.h"
#include "Quad.h"

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

    std::string fileName = "untitled.fbx";
    if (argc > 1) fileName = argv[1]; 

    Quad quad;
    GBuffer gbuffer(800, 800);
    Shader gbufferShader(ReadTextFile("gbuffer_vert.glsl"), ReadTextFile("gbuffer_frag.glsl"));
    Shader lightingShader(ReadTextFile("lighting_vert.glsl"), ReadTextFile("lighting_frag.glsl"));
    Shader forwardShader(ReadTextFile("forward_vertex.glsl"), ReadTextFile("forward_fragment.glsl"));

    Scene scene(fileName);

    Camera camera = scene.camera;
    camera.UpdateDirectionVectors();

    lightingShader.Use();
    lightingShader.SetValue("ambientStrength", 0.1f);
    lightingShader.SetValue("ambientColor", 1.0f);
    forwardShader.Use();
    forwardShader.SetValue("ambientStrength", 0.1f);
    forwardShader.SetValue("ambientColor", 1.0f);

    // sf::Clock clock{};
    while (window.isOpen()){
        // float deltaTime = clock.restart().asSeconds();

        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Rendering
        //-----------------------------------
        // 1. Deferred G-buffer pass
        //-----------------------------------
        gbuffer.BindForWriting();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gbufferShader.Use();
        gbufferShader.SetValue("view", camera.GetViewMatrix());
        gbufferShader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));

        scene.DrawDeferred(gbufferShader);

        gbuffer.BindForReading();

        //-----------------------------------
        // 2. Deferred Lighting Pass
        //-----------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window.getSize().x, window.getSize().y);
        lightingShader.Use();
        lightingShader.SetValue("viewPos", camera.position);
        scene.SetLights(lightingShader);
        gbuffer.BindTextures(lightingShader.programID);
        glDisable(GL_DEPTH_TEST);
        quad.Draw();
        glEnable(GL_DEPTH_TEST);


        //-----------------------------------
        // 3. Forward Pass (overlays)
        //-----------------------------------
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        forwardShader.Use();
        forwardShader.SetValue("view", camera.GetViewMatrix());
        forwardShader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));
        forwardShader.SetValue("viewPos", camera.position);

        scene.DrawForward(forwardShader);

        glDisable(GL_BLEND);

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