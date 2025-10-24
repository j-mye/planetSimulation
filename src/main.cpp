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

    // Massive central star (position, velocity, mass, radius)
    Planet star(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f), 1000.0f, 0.2f);
    star.setColor(glm::vec3(0.95f, 0.85f, 0.30f)); // warm yellow
    planets.push_back(star);

    // Helper to set circular orbital velocity around the star
    auto setCircularOrbitVelocity = [](Planet& body, const Vector2& centerPos, float centerMass, float G, int directionSign = +1) {
        Vector2 rVec = body.getP() - centerPos;
        float r = rVec.length();
        if (r <= 1e-6f) return; // avoid degenerate
        float vMag = std::sqrt(G * centerMass / r);
        // Perpendicular vector (2D): rotate rVec by +90 degrees for tangential direction
        Vector2 tangent(-rVec.getY(), rVec.getX());
        Vector2 v = tangent.normalized() * (vMag * static_cast<float>(directionSign));
        body.setV(v);
    };

    const float G = 0.05f;        // gravitational constant tuned for stability
    const float softening = 0.02f; // force softening to avoid singularities

    // Planet 1: Close orbit at radius 0.8
    Planet planet1(Vector2(0.0f, 0.8f), Vector2(0.0f, 0.0f), 5.0f, 0.05f);
    planet1.setColor(glm::vec3(0.95f, 0.85f, 0.30f)); // warm yellow
    setCircularOrbitVelocity(planet1, star.getP(), star.getMass(), G, +1);
    planets.push_back(planet1);

    // Planet 2: Medium orbit at radius 1.5
    Planet planet2(Vector2(0.0f, 1.5f), Vector2(0.0f, 0.0f), 3.0f, 0.04f);
    planet2.setColor(glm::vec3(0.50f, 0.80f, 1.00f)); // cyan-blue
    setCircularOrbitVelocity(planet2, star.getP(), star.getMass(), G, +1);
    planets.push_back(planet2);

    // Planet 3: Outer orbit at radius 2.5
    Planet planet3(Vector2(0.0f, 2.5f), Vector2(0.0f, 0.0f), 2.0f, 0.03f);
    planet3.setColor(glm::vec3(0.90f, 0.40f, 0.40f)); // soft red
    setCircularOrbitVelocity(planet3, star.getP(), star.getMass(), G, +1);
    planets.push_back(planet3);

    // Moon of planet 1 at radius 0.9 (around origin for simplicity)
    Planet moon(Vector2(0.0f, 0.9f), Vector2(0.0f, 0.0f), 0.5f, 0.02f);
    moon.setColor(glm::vec3(0.85f, 0.85f, 0.95f));   // pale white
    setCircularOrbitVelocity(moon, star.getP(), star.getMass(), G, +1);
    planets.push_back(moon);

    cout << "Planetary Simulation Started!" << endl;
    cout << "Controls:" << endl;
    cout << "  Mouse: Drag to pan, Scroll to zoom" << endl;
    cout << "  Arrow Keys: Pan camera" << endl;
    cout << "  +/-: Zoom in/out" << endl;
    cout << "  Z: Toggle auto-zoom (keeps all planets visible)" << endl;
    cout << "  T: Toggle trails" << endl;
    cout << "  ESC: Exit" << endl;

    // Main simulation loop
    float physicsTimeStep = 0.0015f; // Smaller fixed physics step for stability
    double lastTime = glfwGetTime();
    float time = 0.0f;
    
    while (!renderer.shouldClose()) {
        // Compute frame delta time for smooth camera interpolation
        double now = glfwGetTime();
        float deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;

        // Update physics simulation (fixed time step)
        for (size_t i = 0; i < planets.size(); ++i) {
            Vector2 totalForce(0.0f, 0.0f);
            
            // Calculate gravitational forces from all other planets
            for (size_t j = 0; j < planets.size(); ++j) {
                if (i != j) {
                    Vector2 diff = planets[j].getP() - planets[i].getP();
                    float dist2 = diff.getX()*diff.getX() + diff.getY()*diff.getY() + softening*softening;
                    float invDist = 1.0f / std::sqrt(dist2);
                    float invDist3 = invDist * invDist * invDist;
                    // Gravitational force magnitude with softening: F = G*m1*m2 / r^3 * r
                    float scalar = G * planets[i].getMass() * planets[j].getMass() * invDist3;
                    Vector2 forceVector = diff * scalar; // since diff already has r, multiplying by 1/r^3 gives r/r^3 = 1/r^2
                    totalForce += forceVector;
                }
            }
            
            // Apply force and update position
            planets[i].applyForce(totalForce, physicsTimeStep);
            planets[i].setP(planets[i].getP() + planets[i].getV() * physicsTimeStep);
            planets[i].recordPosition(); // Record for trail
        }
        
        // Update camera to follow center of mass (use frame delta time for smoothness)
        camera.update(planets, deltaTime);
        
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
        renderer.drawBackground(camera);
        renderer.drawTrails(planets, camera);
        renderer.drawPlanets(planets, camera);
        renderer.endFrame();
        
        time += deltaTime;
    }

    // Cleanup
    renderer.cleanup();
    
    cout << "Simulation ended after " << time << " seconds" << endl;
    return 0;
}
