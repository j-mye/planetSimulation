#ifndef GUI_HPP
#define GUI_HPP

#include <GLFW/glfw3.h>
#include <deque>
#include "planets/Simulation.hpp"
#include "Camera.hpp"

class Renderer; // forward declaration

/**
 * @brief GUI overlay for simulation control and statistics display
 * Uses ImGui for rendering UI elements
 */
class GUI {
private:
    GLFWwindow* window;
    bool visible = true;
    static constexpr int PANEL_WIDTH = 320;
    
    // FPS tracking
    std::deque<float> fpsHistory;
    static constexpr size_t FPS_HISTORY_SIZE = 100;
    
    // Control state
    bool isPaused = false;
    float timeScale = 1.0f;
    
    // Base physics constants
    static constexpr float BASE_GRAVITY = 0.05f;
    static constexpr float BASE_SOFTENING = 0.02f;
    float gravityMultiplier = 1.0f;
    float softeningMultiplier = 1.0f;
    
    // Stats
    int lastPlanetCount = 0;
    float lastFPS = 0.0f;

public:
    GUI();
    ~GUI();
    
    bool init(GLFWwindow* win);
    void shutdown();
    
    void newFrame();
    void render(Simulation& sim, Camera& camera, Renderer& renderer, float deltaTime);
    
    // Control getters
    bool isSimulationPaused() const { return isPaused; }
    float getTimeScale() const { return timeScale; }
    bool isVisible() const { return visible; }
    int getPanelWidth() const { return PANEL_WIDTH; }
    
    // Ask whether ImGui currently wants to capture keyboard input
    bool wantsCaptureKeyboard() const;
    bool wantsCaptureMouse() const;
    
    void setPaused(bool paused) { isPaused = paused; }
    void toggleVisibility() { visible = !visible; }

    // Restart signalling for main loop (to reset accumulators, etc.)
    bool wasRestartTriggered() const { return restartTriggered; }
    void clearRestartFlag() { restartTriggered = false; }

private:
    bool restartTriggered = false;
};

#endif // GUI_HPP
