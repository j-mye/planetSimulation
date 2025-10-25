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
    // Active simulation viewport (in framebuffer pixels, origin bottom-left)
    int vpLeft = 0, vpBottom = 0, vpWidth = 0, vpHeight = 0;
    
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
    
    // Static starfield rendering
    GLuint starVAO = 0;
    GLuint starVBO = 0;
    GLuint starShaderProgram = 0;
    int starCount = 0;
    
    // Camera system
    glm::vec2 cameraPosition;
    glm::mat4 viewMatrix;
    
    // Trail system
    std::vector<std::deque<Vector2>> trails;
    bool trailsEnabled;
    int maxTrailLength;
    
    // Background toggle
    bool starfieldEnabled;
    
    void updateViewMatrix();
    void updateTrails(const std::vector<Planet>& planets);
    void setupInputCallbacks();
    void initStarfield();
    void drawStarfield();

public:
    Renderer(int w, int h, const char* title);
    
    bool init();
    void beginFrame();
    void drawBackground(const Camera& camera);
    void drawPlanets(const std::vector<Planet>& planets, const Camera& camera);
    void drawTrails(const std::vector<Planet>& planets, const Camera& camera);
    void endFrame();
    bool shouldClose();
    void cleanup();
    GLFWwindow* getWindow() const { return window; }
    void setZoom(float zoom);
    void pan(float dx, float dy);

    glm::mat4 getViewMatrix() const { return viewMatrix; }
    void setTrailsEnabled(bool enabled) { trailsEnabled = enabled; }
    bool areTrailsEnabled() const { return trailsEnabled; }
    void clearTrails();
    void handleInput();
    void setViewportRect(int left, int bottom, int width, int height) { vpLeft = left; vpBottom = bottom; vpWidth = width; vpHeight = height; }
    bool isInsideViewport(double xWindow, double yWindow) const;
    
    // Background control
    void setStarfieldEnabled(bool enabled) { starfieldEnabled = enabled; }
    bool isStarfieldEnabled() const { return starfieldEnabled; }
    
    // Input handling (public for callback access)
    bool mousePressed;
    glm::vec2 lastMousePos;
    float cameraZoom;
};

#endif // RENDERER_HPP
