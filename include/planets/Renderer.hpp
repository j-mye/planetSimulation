#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <deque>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Planet.hpp"
#include "Camera.hpp"

/**
 * @brief Enhanced OpenGL renderer for planetary simulation with camera, glow effects, and trails
 */
class Renderer {
private:
    int width;
    int height;
    GLFWwindow* window;
    
    // Planet rendering
    GLuint planetVAO;
    GLuint planetVBO;
    GLuint planetShaderProgram;
    GLint loc_uView;
    GLint loc_uPointSize;
    GLint loc_uColor;
    
    // Trail rendering
    GLuint trailVAO;
    GLuint trailVBO;
    GLuint trailShaderProgram;
    GLint trailLoc_uView;
    GLint trailLoc_uColor;
    
    // Background rendering
    GLuint backgroundVAO;
    GLuint backgroundVBO;
    GLuint backgroundShaderProgram;
    
    // Camera system
    glm::vec2 cameraPosition;
    glm::mat4 viewMatrix;
    
    // Trail system
    std::vector<std::deque<Vector2>> trails;
    bool trailsEnabled;
    int maxTrailLength;
    
    // Helper methods
    void updateViewMatrix();
    void updateTrails(const std::vector<Planet>& planets);
    void setupInputCallbacks();

public:
    /**
     * @brief Constructor for the renderer
     * @param w Window width
     * @param h Window height
     * @param title Window title
     */
    Renderer(int w, int h, const char* title);
    
    /**
     * @brief Initialize OpenGL context, shaders, and buffers
     * @return true if initialization successful, false otherwise
     */
    bool init();
    
    /**
     * @brief Begin a new frame (clear screen)
     */
    void beginFrame();
    
    /**
     * @brief Draw space gradient background
     */
    void drawBackground();
    
    /**
     * @brief Draw planets with glow effects
     * @param planets Vector of Planet objects to render
     * @param camera Camera object for view matrix
     */
    void drawPlanets(const std::vector<Planet>& planets, const Camera& camera);
    
    /**
     * @brief Draw planetary trails
     * @param planets Vector of Planet objects to render trails for
     * @param camera Camera object for view matrix
     */
    void drawTrails(const std::vector<Planet>& planets, const Camera& camera);
    
    /**
     * @brief End frame (swap buffers and poll events)
     */
    void endFrame();
    
    /**
     * @brief Check if window should close
     * @return true if window should close
     */
    bool shouldClose();
    
    /**
     * @brief Clean up OpenGL resources
     */
    void cleanup();
    
    /**
     * @brief Get the GLFW window pointer
     * @return GLFWwindow* pointer
     */
    GLFWwindow* getWindow() const { return window; }
    
    // Camera controls
    /**
     * @brief Set camera zoom level
     * @param zoom Zoom factor (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
     */
    void setZoom(float zoom);
    
    /**
     * @brief Pan camera by offset
     * @param dx X offset
     * @param dy Y offset
     */
    void pan(float dx, float dy);
    
    /**
     * @brief Get current view matrix
     * @return Current view matrix
     */
    glm::mat4 getViewMatrix() const { return viewMatrix; }
    
    /**
     * @brief Enable or disable trail rendering
     * @param enabled Whether trails should be drawn
     */
    void setTrailsEnabled(bool enabled) { trailsEnabled = enabled; }
    
    /**
     * @brief Check if trails are enabled
     * @return true if trails are enabled
     */
    bool areTrailsEnabled() const { return trailsEnabled; }
    
    /**
     * @brief Clear all trails
     */
    void clearTrails();
    
    /**
     * @brief Handle input events (call this each frame)
     */
    void handleInput();
    
    // Input handling (public for callback access)
    bool mousePressed;
    glm::vec2 lastMousePos;
    float cameraZoom;
};

#endif // RENDERER_HPP
