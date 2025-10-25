#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include <vector>
#include <deque>
#include "Planet.hpp"
#include "Simulation.hpp"
#include "Camera.hpp"

/**
 * @brief GUI overlay for simulation control and statistics display
 * Uses ImGui for rendering UI elements
 */
class GUI {
private:
    GLFWwindow* window;
    bool visible = true;
    
    // FPS tracking
    std::deque<float> fpsHistory;
    static constexpr size_t FPS_HISTORY_SIZE = 100;
    
    // Control state
    bool isPaused = false;
    float timeScale = 1.0f;
    
    // Stats
    int lastPlanetCount = 0;
    float lastFPS = 0.0f;

public:
    GUI();
    ~GUI();
    
    bool init(GLFWwindow* win);
    void shutdown();
    
    void newFrame();
    void render(Simulation& sim, Camera& camera, float deltaTime);
    
    // Control getters
    bool isSimulationPaused() const { return isPaused; }
    float getTimeScale() const { return timeScale; }
    bool isVisible() const { return visible; }
    
    // Control setters
    void setPaused(bool paused) { isPaused = paused; }
    void toggleVisibility() { visible = !visible; }
};

#endif // GUI_HPP
