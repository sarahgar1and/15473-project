#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

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
    Mode mode = HYBRID;
    if (argc > 1) fileName = argv[1]; 
    if (argc > 2){
        std::string modeArg(argv[2]);
        if (modeArg == "d" || modeArg == "deferred") mode = DEFERRED;
        else if (modeArg == "f" || modeArg == "forward") mode = FORWARD;
        else if (modeArg == "h" || modeArg == "hybrid") mode = HYBRID;
    }

    Quad quad;
    GBuffer gbuffer((int)window.getSize().x, (int)window.getSize().y);
    Shader gbufferShader(ReadTextFile("gbuffer_vert.glsl"), ReadTextFile("gbuffer_frag.glsl"));
    Shader lightingShader(ReadTextFile("lighting_vert.glsl"), ReadTextFile("lighting_frag.glsl"));
    Shader forwardShader(ReadTextFile("forward_vertex.glsl"), ReadTextFile("forward_fragment.glsl"));

    Scene scene(fileName);

    Camera camera = scene.camera;
    camera.UpdateDirectionVectors();
    
    // Set up view/projection for overdraw measurement
    gbufferShader.Use();
    gbufferShader.SetValue("view", camera.GetViewMatrix());
    gbufferShader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));
    
    // Measure preprocessing time (overdraw detection and heuristic evaluation)
    sf::Clock preprocessClock;
    scene.UpdateRenderingMode(gbufferShader, window.getSize().x, window.getSize().y, mode);
    float preprocessTime = preprocessClock.getElapsedTime().asSeconds() * 1000.0f; // Convert to milliseconds

    lightingShader.Use();
    lightingShader.SetValue("viewPos", camera.position);
    lightingShader.SetValue("ambientStrength", 0.1f);
    lightingShader.SetValue("ambientColor", glm::vec3(1.0f));
    forwardShader.Use();
    forwardShader.SetValue("ambientStrength", 0.1f);
    forwardShader.SetValue("ambientColor", glm::vec3(1.0f));
    forwardShader.SetValue("view", camera.GetViewMatrix());
    forwardShader.SetValue("projection", camera.GetProjectionMatrix((float)window.getSize().x, (float)window.getSize().y));
    forwardShader.SetValue("viewPos", camera.position);


    // Benchmarking configuration
    const int WARMUP_FRAMES = 10;  // Skip first N frames for warm-up
    const int SAMPLE_FRAMES = 100; // Collect M frames for statistics
    std::vector<float> renderTimes;
    renderTimes.reserve(SAMPLE_FRAMES);
    
    int frameCount = 0;
    sf::Clock clock{};
    bool statsPrinted = false;
    
    while (window.isOpen()){
        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //-----------------------------------
        // 1. Deferred G-buffer pass
        //-----------------------------------
        gbuffer.BindForWriting();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gbufferShader.Use();

        // Ensure previous frame is complete before timing
        if (frameCount >= WARMUP_FRAMES) {
            glFinish();
            clock.restart(); // Start timing the actual rendering work
        }
        
        int deferredCount = scene.DrawDeferred(gbufferShader);

        //-----------------------------------
        // 2. Deferred Lighting Pass
        //-----------------------------------
        if (deferredCount > 0) {
            gbuffer.BindForReading();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, window.getSize().x, window.getSize().y);
            
            glDisable(GL_DEPTH_TEST);
            
            lightingShader.Use();
            scene.SetLights(lightingShader);
            gbuffer.BindTextures(lightingShader.programID);
            quad.Draw();
            
            glEnable(GL_DEPTH_TEST);
        } else {
            // Skip lighting pass if no deferred objects
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, window.getSize().x, window.getSize().y);
        }


        //-----------------------------------
        // 3. Forward Pass 
        //-----------------------------------
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        forwardShader.Use();

        int forwardCount = scene.DrawForward(forwardShader);

        glDisable(GL_BLEND);

        // Collect render time sample (after warm-up period)
        if (frameCount >= WARMUP_FRAMES && frameCount < WARMUP_FRAMES + SAMPLE_FRAMES) {
            glFinish(); // Ensure GPU work is complete
            float renderTime = clock.getElapsedTime().asSeconds() * 1000.0f; // Convert to milliseconds
            renderTimes.push_back(renderTime);
        }
        
        frameCount++;

        // Calculate and print statistics after collecting all samples
        if (!statsPrinted && frameCount >= WARMUP_FRAMES + SAMPLE_FRAMES) {
            // Calculate statistics
            if (!renderTimes.empty()) {
                std::sort(renderTimes.begin(), renderTimes.end());
                
                // Mean
                float sum = 0.0f;
                for (float time : renderTimes) {
                    sum += time;
                }
                float mean = sum / renderTimes.size();
                
                // Median
                float median = renderTimes.size() % 2 == 0
                    ? (renderTimes[renderTimes.size() / 2 - 1] + renderTimes[renderTimes.size() / 2]) / 2.0f
                    : renderTimes[renderTimes.size() / 2];
                
                // Min/Max
                float minTime = renderTimes.front();
                float maxTime = renderTimes.back();
                
                // Standard deviation
                float variance = 0.0f;
                for (float time : renderTimes) {
                    float diff = time - mean;
                    variance += diff * diff;
                }
                float stdDev = std::sqrt(variance / renderTimes.size());
                
                float gbufferMemory = gbuffer.GetMemoryUsageMB();
                std::cout << "Render Stats - Deferred: " << deferredCount 
                          << " objects, Forward: " << forwardCount 
                          << " objects" << std::endl;
                std::cout << "Render time: mean=" << mean << " ms (median=" << median 
                          << ", stddev=" << stdDev << ", min=" << minTime 
                          << ", max=" << maxTime << ")" << std::endl;
                std::cout << "Preprocess time: " << preprocessTime << " ms"
                          << ", G-buffer memory: " << gbufferMemory << " MB" << std::endl;
            }
            statsPrinted = true;
        }

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