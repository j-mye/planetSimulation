#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "planets/Renderer.hpp"
#include "planets/Planet.hpp"
#include "planets/Vector2.hpp"
#include "planets/Camera.hpp"
#include "planets/Simulation.hpp"

using namespace std;

int main() {
    Renderer renderer(1280, 720, "Planetary Simulation");
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer" << endl;
        return -1;
    }

    Camera camera(1280.0f, 720.0f);

    // Create simulation and initial random bodies
    Simulation sim;
    sim.setGravityParams(0.05f, 0.02f);
    sim.setTimeStep(0.0015f);
    const int N = 12;
    sim.initRandom(N, 1337);

    cout << "Planetary Simulation Started!" << endl;
    cout << "Controls:" << endl;
    cout << "  Mouse: Drag to pan, Scroll to zoom" << endl;
    cout << "  Arrow Keys: Pan camera" << endl;
    cout << "  +/-: Zoom in/out" << endl;
    cout << "  Z: Toggle auto-zoom (keeps all planets visible)" << endl;
    cout << "  T: Toggle trails" << endl;
    cout << "  ESC: Exit" << endl;

    float physicsTimeStep = 0.0016f;
    double lastTime = glfwGetTime();
    float time = 0.0f;
    
    while (!renderer.shouldClose()) {
        double now = glfwGetTime();
        float deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;

        // advance physics (fixed step)
        sim.step();

        // camera follows simulation planets
        camera.update(sim.getPlanets(), deltaTime);
        renderer.handleInput();
        
    GLFWwindow* window = renderer.getWindow();
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            camera.zoomBy(1.05f);
        }
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
            camera.zoomBy(0.95f);
        }
        
        float panSpeed = 0.01f / camera.getZoom();
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            camera.pan(-panSpeed, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            camera.pan(panSpeed, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            camera.pan(0.0f, panSpeed);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            camera.pan(0.0f, -panSpeed);
        }
        
        static bool zKeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !zKeyPressed) {
            camera.setAutoZoom(!camera.isAutoZoomEnabled());
            std::cout << "Auto-zoom " << (camera.isAutoZoomEnabled() ? "enabled" : "disabled") << std::endl;
            zKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
            zKeyPressed = false;
        }
        
    renderer.beginFrame();
    renderer.drawBackground(camera);
    renderer.drawTrails(sim.getPlanets(), camera);
    renderer.drawPlanets(sim.getPlanets(), camera);
        renderer.endFrame();
        time += deltaTime;
    }
    
    renderer.cleanup();    
    cout << "Simulation ended after " << time << " seconds" << endl;
    return 0;
}
