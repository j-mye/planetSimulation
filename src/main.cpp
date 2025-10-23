#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "planets/Renderer.hpp"
#include "planets/Planet.hpp"
#include "planets/Vector2.hpp"
#include "planets/Camera.hpp"

using namespace std;

int main() {
    // Initialize the enhanced renderer
    Renderer renderer(1280, 720, "Planetary Simulation");
    
    if (!renderer.init()) {
        cout << "Failed to initialize renderer" << endl;
        return -1;
    }

    // Initialize the camera system
    Camera camera(1280.0f, 720.0f);

    // Create a planetary system
    vector<Planet> planets;
    
    // Central star (large mass at origin)
    Planet star(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
    star.setMass(1000.0f);
    star.setRadius(0.2f);
    planets.push_back(star);
    
    // Planet 1: Close orbit (reduced velocity for stability)
    Planet planet1(Vector2(0.0f, 0.8f), Vector2(0.1f, 0.0f));
    planet1.setMass(5.0f);
    planet1.setRadius(0.05f);
    planets.push_back(planet1);
    
    // Planet 2: Medium orbit (reduced velocity for stability)
    Planet planet2(Vector2(0.0f, 1.5f), Vector2(0.08f, 0.0f));
    planet2.setMass(3.0f);
    planet2.setRadius(0.04f);
    planets.push_back(planet2);
    
    // Planet 3: Outer orbit (reduced velocity for stability)
    Planet planet3(Vector2(0.0f, 2.5f), Vector2(0.05f, 0.0f));
    planet3.setMass(2.0f);
    planet3.setRadius(0.03f);
    planets.push_back(planet3);
    
    // Moon of planet 1 (reduced velocity for stability)
    Planet moon(Vector2(0.0f, 0.9f), Vector2(0.12f, 0.0f));
    moon.setMass(0.5f);
    moon.setRadius(0.02f);
    planets.push_back(moon);

    cout << "Planetary Simulation Started!" << endl;
    cout << "Controls:" << endl;
    cout << "  Mouse: Drag to pan, Scroll to zoom" << endl;
    cout << "  Arrow Keys: Pan camera" << endl;
    cout << "  +/-: Zoom in/out" << endl;
    cout << "  Z: Toggle auto-zoom (keeps all planets visible)" << endl;
    cout << "  T: Toggle trails" << endl;
    cout << "  ESC: Exit" << endl;
    cout << "  Camera automatically follows center of mass!" << endl;

    // Main simulation loop
    float timeStep = 0.000001f; // ~60 FPS
    float time = 0.0f;
    
    while (!renderer.shouldClose()) {
        // Update physics simulation
        for (size_t i = 0; i < planets.size(); ++i) {
            Vector2 totalForce(0.0f, 0.0f);
            
            // Calculate gravitational forces from all other planets
            for (size_t j = 0; j < planets.size(); ++j) {
                if (i != j) {
                    Vector2 diff = planets[j].getP() - planets[i].getP();
                    float distance = diff.length();
                    
                    if (distance > 0.01f) { // Avoid division by zero
                        // Proper gravitational force: F = G * m1 * m2 / r^2
                        // Using G = 1 for simplicity, but need proper scaling
                        float force = (planets[i].getMass() * planets[j].getMass()) / (distance * distance);
                        Vector2 forceVector = diff.normalized() * force;
                        totalForce += forceVector;
                    }
                }
            }
            
            // Apply force and update position
            planets[i].applyForce(totalForce, timeStep);
            planets[i].setP(planets[i].getP() + planets[i].getV() * timeStep);
            planets[i].recordPosition(); // Record for trail
        }
        
        // Update camera to follow center of mass
        camera.update(planets, timeStep);
        
        // Handle input
        renderer.handleInput();
        
        // Handle camera zoom input
        GLFWwindow* window = renderer.getWindow();
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            camera.zoomBy(1.05f);
        }
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
            camera.zoomBy(0.95f);
        }
        
        // Handle camera pan input
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
        
        // Toggle auto-zoom with Z key
        static bool zKeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && !zKeyPressed) {
            camera.setAutoZoom(!camera.isAutoZoomEnabled());
            std::cout << "Auto-zoom " << (camera.isAutoZoomEnabled() ? "enabled" : "disabled") << std::endl;
            zKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
            zKeyPressed = false;
        }
        
        // Render frame
        renderer.beginFrame();
        renderer.drawBackground();
        renderer.drawTrails(planets, camera);
        renderer.drawPlanets(planets, camera);
        renderer.endFrame();
        
        time += timeStep;
    }

    // Cleanup
    renderer.cleanup();
    
    cout << "Simulation ended after " << time << " seconds" << endl;
    return 0;
}
