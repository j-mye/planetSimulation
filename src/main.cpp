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
#include "planets/GUI.hpp"

using namespace std;

int main() {
    Renderer renderer(1280, 720, "Planetary Simulation");
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer" << endl;
        return -1;
    }

    Camera camera(1280.0f, 720.0f);

    // Initialize GUI
    GUI gui;
    if (!gui.init(renderer.getWindow())) {
        std::cerr << "Failed to initialize GUI" << endl;
        return -1;
    }

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
    cout << "  T: Toggle trails" << endl;
    cout << "  H: Toggle GUI panel" << endl;
    cout << "  C: Clear trails" << endl;
    cout << "  SPACE: Pause/Resume simulation" << endl;
    cout << "  ESC: Exit" << endl;

    float physicsTimeStep = 0.0016f;
    double lastTime = glfwGetTime();
    float time = 0.0f;
    
    
    while (!renderer.shouldClose()) {
        double now = glfwGetTime();
        float deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;

    // Handle input
    GLFWwindow* window = renderer.getWindow();
        
        // Query whether GUI wants input focus; if so, defer most shortcuts and camera input
        bool guiCapturesKeyboard = gui.wantsCaptureKeyboard();
        bool guiCapturesMouse = gui.wantsCaptureMouse();

        // Compute simulation viewport (right side of window) based on GUI panel width
        int fbW = 0, fbH = 0; glfwGetFramebufferSize(window, &fbW, &fbH);
        int winW = 0, winH = 0; glfwGetWindowSize(window, &winW, &winH);
        float xscale = (winW > 0) ? (float)fbW / (float)winW : 1.0f;
        int simLeft = 0;
        if (gui.isVisible()) {
            simLeft = (int)std::round(gui.getPanelWidth() * xscale);
        }
        int simBottom = 0;
        int simWidth = std::max(0, fbW - simLeft);
        int simHeight = fbH;

        // Apply viewport to the renderer for input hit-testing
        // (Rendering will set viewport before drawing)
        renderer.setViewportRect(simLeft, simBottom, simWidth, simHeight);
        // Defer renderer keyboard/mouse input handling if GUI is capturing inputs
        if (!guiCapturesKeyboard && !guiCapturesMouse) {
            renderer.handleInput();
        }

        // Double-click to follow planet (only if not captured by GUI)
        if (!guiCapturesMouse) {
            static double lastClickTime = 0.0;
            static const double doubleClickDelay = 0.3; // 300ms
            
            static bool mouseButtonWasPressed = false;
            bool mouseButtonPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
            
            if (mouseButtonPressed && !mouseButtonWasPressed) {
                // Mouse button just pressed
                double currentTime = glfwGetTime();
                if (currentTime - lastClickTime < doubleClickDelay) {
                    // Double-click detected!
                    double xpos, ypos;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    
                    // Convert screen coordinates to world coordinates
                    // Account for GUI panel offset
                    float xNorm = ((float)xpos - (float)gui.getPanelWidth()) / (float)(winW - gui.getPanelWidth());
                    float yNorm = (float)ypos / (float)winH;
                    
                    // Convert to NDC [-1, 1]
                    float ndcX = xNorm * 2.0f - 1.0f;
                    float ndcY = 1.0f - yNorm * 2.0f; // flip Y
                    
                    // Convert to world space
                    glm::vec2 worldPos = camera.getPosition() + glm::vec2(ndcX, ndcY) / camera.getZoom();
                    
                    // Find closest planet
                    int closestIndex = -1;
                    float closestDist = std::numeric_limits<float>::max();
                    for (int i = 0; i < sim.getPlanets().size(); ++i) {
                        const Planet& p = sim.getPlanets()[i];
                        glm::vec2 planetPos(p.getP().getX(), p.getP().getY());
                        float dist = glm::length(worldPos - planetPos);
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestIndex = i;
                        }
                    }
                    
                    // Toggle follow mode
                    if (closestIndex >= 0) {
                        if (camera.getFollowedPlanet() == closestIndex) {
                            // Double-clicked same planet - return to COM follow
                            camera.setFollowedPlanet(-1);
                            std::cout << "Following center of mass" << std::endl;
                        } else {
                            // Follow new planet
                            camera.setFollowedPlanet(closestIndex);
                            std::cout << "Following planet " << closestIndex << std::endl;
                        }
                    }
                    
                    lastClickTime = 0.0; // reset to prevent triple-click
                } else {
                    lastClickTime = currentTime;
                }
            }
            mouseButtonWasPressed = mouseButtonPressed;
        }

    // GUI toggle (H) should always be available
        
    // GUI controls
        static bool hKeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hKeyPressed) {
            gui.toggleVisibility();
            hKeyPressed = true;
        } else if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
            hKeyPressed = false;
        }
        
        if (!guiCapturesKeyboard) {
            static bool spaceKeyPressed = false;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spaceKeyPressed) {
                gui.setPaused(!gui.isSimulationPaused());
                std::cout << "Simulation " << (gui.isSimulationPaused() ? "paused" : "resumed") << std::endl;
                spaceKeyPressed = true;
            } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
                spaceKeyPressed = false;
            }
            
            static bool cKeyPressed = false;
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
                renderer.clearTrails();
                std::cout << "Trails cleared" << std::endl;
                cKeyPressed = true;
            } else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
                cKeyPressed = false;
            }
        }

        // Advance physics using fixed-step accumulator
        {
            static double accumulator = 0.0;
            if (gui.wasRestartTriggered()) {
                accumulator = 0.0; // avoid heavy catch-up after restart
                gui.clearRestartFlag();
            }

            if (!gui.isSimulationPaused()) {
                const float baseSimDt = sim.getTimeStep();
                accumulator += deltaTime;

                // Clamp accumulator to prevent spiral of death on slow frames
                const double maxAccum = 0.25; // seconds
                if (accumulator > maxAccum) accumulator = maxAccum;

                int substeps = 0;
                const int maxSubstepsPerFrame = 500;
                while (accumulator >= baseSimDt && substeps < maxSubstepsPerFrame) {
                    // Apply time scaling by adjusting dt passed to simulation
                    const float scaledDt = baseSimDt * gui.getTimeScale();
                    sim.setTimeStep(scaledDt);
                    sim.step();
                    sim.setTimeStep(baseSimDt); // restore base timestep
                    accumulator -= baseSimDt;
                    ++substeps;
                }
            }
        }

        // Camera follows simulation planets
        camera.update(sim.getPlanets(), deltaTime);
        // Manual camera controls (only when GUI is not capturing keyboard input)
        if (!guiCapturesKeyboard) {
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
        }
        
        // Render
        gui.newFrame();

        renderer.beginFrame();
        // Set viewport for simulation drawing (right pane)
        glViewport(simLeft, simBottom, simWidth, simHeight);
        renderer.drawBackground(camera);
        renderer.drawTrails(sim.getPlanets(), camera);
        renderer.drawPlanets(sim.getPlanets(), camera);
        
        // Reset viewport to full window for GUI draw
        glViewport(0, 0, fbW, fbH);
        gui.render(sim, camera, renderer, deltaTime);
    
        renderer.endFrame();
        time += deltaTime;
    }
    
    gui.shutdown();
    renderer.cleanup();    
    cout << "Simulation ended after " << time << " seconds" << endl;
    return 0;
}
