#include <SFML/Graphics.hpp>
#define GL_SILENCE_DEPRECATION
#include <SFML/OpenGL.hpp>

int main(){
    sf::Window window(sf::VideoMode({1200, 900}), "3D OpenGL");

    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);

    while(window.isOpen()){
        while (const std::optional event = window.pollEvent()){
            if (event->is<sf::Event::Closed>())
                window.close();
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // draw

         window.display();

    }
}