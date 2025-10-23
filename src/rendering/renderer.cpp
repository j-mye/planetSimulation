#include "planets/Renderer.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

// Planet shader sources
static const char* planetVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in float aMass;
layout(location = 2) in vec3 aVelocity;

out vec3 vColor;

uniform mat4 uView;
void main() {
    gl_Position = uView * vec4(aPos, 0.0, 1.0);
    gl_PointSize = aMass * 3.0;
    float speed = length(aVelocity);
    vColor = vec3(speed, 0.3, 1.0 - speed);
}
)";

static const char* planetFragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    float intensity = exp(-6.0 * dist * dist);
    FragColor = vec4(vColor * intensity, intensity);
}
)";

// Trail shader sources
static const char* trailVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in float aAge;

out float vAge;

uniform mat4 uView;
void main() {
    gl_Position = uView * vec4(aPos, 0.0, 1.0);
    vAge = aAge;
}
)";

static const char* trailFragmentShaderSrc = R"(
#version 330 core
in float vAge;
out vec4 FragColor;

uniform vec3 uColor;
void main() {
    float alpha = 1.0 - vAge;
    FragColor = vec4(uColor, alpha);
}
)";

// Background shader sources
static const char* backgroundVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

out vec2 uv;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    uv = (aPos + 1.0) * 0.5;
}
)";

static const char* backgroundFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec2 uv;
void main() {
    vec3 top = vec3(0.02, 0.02, 0.07);
    vec3 bottom = vec3(0.0, 0.0, 0.0);
    FragColor = vec4(mix(bottom, top, uv.y), 1.0);
}
)";

// Helper: compile shader and print errors
static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}

// Helper: link program and print errors
static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    return prog;
}

// Static callback functions for GLFW
static Renderer* currentRenderer = nullptr;

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (currentRenderer) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            currentRenderer->mousePressed = (action == GLFW_PRESS);
        }
    }
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (currentRenderer && currentRenderer->mousePressed) {
        glm::vec2 currentMousePos(xpos, ypos);
        glm::vec2 delta = currentMousePos - currentRenderer->lastMousePos;
        currentRenderer->pan(delta.x * 0.01f, -delta.y * 0.01f);
        currentRenderer->lastMousePos = currentMousePos;
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentRenderer) {
        float zoomFactor = 1.0f + yoffset * 0.1f;
        currentRenderer->setZoom(currentRenderer->cameraZoom * zoomFactor);
    }
}

Renderer::Renderer(int w, int h, const char* title)
    : width(w), height(h), window(nullptr), 
      planetVAO(0), planetVBO(0), planetShaderProgram(0),
      trailVAO(0), trailVBO(0), trailShaderProgram(0),
      backgroundVAO(0), backgroundVBO(0), backgroundShaderProgram(0),
      cameraPosition(0.0f, 0.0f), cameraZoom(1.0f),
      trailsEnabled(true), maxTrailLength(500),
      mousePressed(false), lastMousePos(0.0f, 0.0f) {
    currentRenderer = this;
}

bool Renderer::init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Planetary Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glViewport(0, 0, width, height);
    
    // Enable blending for glow effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // Enable point size control
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Compile and link planet shaders
    GLuint planetVS = compileShader(GL_VERTEX_SHADER, planetVertexShaderSrc);
    GLuint planetFS = compileShader(GL_FRAGMENT_SHADER, planetFragmentShaderSrc);
    planetShaderProgram = linkProgram(planetVS, planetFS);
    glDeleteShader(planetVS);
    glDeleteShader(planetFS);

    // Get planet shader uniform locations
    loc_uView = glGetUniformLocation(planetShaderProgram, "uView");

    // Compile and link trail shaders
    GLuint trailVS = compileShader(GL_VERTEX_SHADER, trailVertexShaderSrc);
    GLuint trailFS = compileShader(GL_FRAGMENT_SHADER, trailFragmentShaderSrc);
    trailShaderProgram = linkProgram(trailVS, trailFS);
    glDeleteShader(trailVS);
    glDeleteShader(trailFS);

    // Get trail shader uniform locations
    trailLoc_uView = glGetUniformLocation(trailShaderProgram, "uView");
    trailLoc_uColor = glGetUniformLocation(trailShaderProgram, "uColor");

    // Compile and link background shaders
    GLuint backgroundVS = compileShader(GL_VERTEX_SHADER, backgroundVertexShaderSrc);
    GLuint backgroundFS = compileShader(GL_FRAGMENT_SHADER, backgroundFragmentShaderSrc);
    backgroundShaderProgram = linkProgram(backgroundVS, backgroundFS);
    glDeleteShader(backgroundVS);
    glDeleteShader(backgroundFS);

    // Create planet VAO and VBO
    glGenVertexArrays(1, &planetVAO);
    glGenBuffers(1, &planetVBO);

    glBindVertexArray(planetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Planet vertex attributes: position (vec2), mass (float), velocity (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // Create trail VAO and VBO
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);

    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Trail vertex attributes: position (vec2), age (float)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));

    // Create background VAO and VBO
    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);

    glBindVertexArray(backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);

    // Fullscreen quad vertices
    float backgroundVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Setup input callbacks
    setupInputCallbacks();

    // Initialize view matrix
    updateViewMatrix();

    return true;
}

void Renderer::setupInputCallbacks() {
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
}

void Renderer::updateViewMatrix() {
    viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::scale(viewMatrix, glm::vec3(cameraZoom, cameraZoom, 1.0f));
    viewMatrix = glm::translate(viewMatrix, glm::vec3(-cameraPosition.x, -cameraPosition.y, 0.0f));
}

void Renderer::setZoom(float zoom) {
    cameraZoom = std::max(0.1f, std::min(10.0f, zoom));
    updateViewMatrix();
}

void Renderer::pan(float dx, float dy) {
    cameraPosition.x += dx / cameraZoom;
    cameraPosition.y += dy / cameraZoom;
    updateViewMatrix();
}

void Renderer::beginFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawBackground() {
    glUseProgram(backgroundShaderProgram);
    glBindVertexArray(backgroundVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::drawPlanets(const std::vector<Planet>& planets, const Camera& camera) {
    if (planets.empty()) return;

    // Extract planet data
    std::vector<float> planetData;
    planetData.reserve(planets.size() * 6); // pos(2) + mass(1) + velocity(3)

    for (const auto& planet : planets) {
        // Position
        planetData.push_back(planet.getP().getX());
        planetData.push_back(planet.getP().getY());
        
        // Mass
        planetData.push_back(planet.getMass());
        
        // Velocity
        planetData.push_back(planet.getV().getX());
        planetData.push_back(planet.getV().getY());
        planetData.push_back(0.0f); // Z component (not used in 2D)
    }

    // Upload data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, planetData.size() * sizeof(float), planetData.data(), GL_DYNAMIC_DRAW);

    // Draw planets using camera's view matrix
    glUseProgram(planetShaderProgram);
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glUniformMatrix4fv(loc_uView, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glBindVertexArray(planetVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(planets.size()));

    // Cleanup
    glBindVertexArray(0);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::updateTrails(const std::vector<Planet>& planets) {
    // Resize trails vector if needed
    if (trails.size() != planets.size()) {
        trails.resize(planets.size());
    }

    // Update trails for each planet
    for (size_t i = 0; i < planets.size(); ++i) {
        trails[i].push_back(planets[i].getP());
        
        // Limit trail length
        while (trails[i].size() > maxTrailLength) {
            trails[i].pop_front();
        }
    }
}

void Renderer::drawTrails(const std::vector<Planet>& planets, const Camera& camera) {
    if (!trailsEnabled || planets.empty()) return;

    updateTrails(planets);

    glUseProgram(trailShaderProgram);
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glUniformMatrix4fv(trailLoc_uView, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    // Draw trails for each planet
    for (size_t i = 0; i < trails.size() && i < planets.size(); ++i) {
        if (trails[i].size() < 2) continue;

        // Prepare trail data
        std::vector<float> trailData;
        trailData.reserve(trails[i].size() * 3); // pos(2) + age(1)

        float maxAge = static_cast<float>(trails[i].size() - 1);
        for (size_t j = 0; j < trails[i].size(); ++j) {
            trailData.push_back(trails[i][j].getX());
            trailData.push_back(trails[i][j].getY());
            trailData.push_back(j / maxAge); // Age from 0 to 1
        }

        // Upload and draw trail
        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
        glBufferData(GL_ARRAY_BUFFER, trailData.size() * sizeof(float), trailData.data(), GL_DYNAMIC_DRAW);

        // Set trail color (light blue)
        glUniform3f(trailLoc_uColor, 0.3f, 0.6f, 1.0f);

        glBindVertexArray(trailVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(trails[i].size()));
    }

    // Cleanup
    glBindVertexArray(0);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Renderer::shouldClose() {
    return glfwWindowShouldClose(window);
}

void Renderer::clearTrails() {
    for (auto& trail : trails) {
        trail.clear();
    }
}

void Renderer::handleInput() {
    // Handle keyboard input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Handle arrow keys for panning
    float panSpeed = 0.01f / cameraZoom;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        pan(-panSpeed, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        pan(panSpeed, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        pan(0.0f, panSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        pan(0.0f, -panSpeed);
    }

    // Handle zoom with +/- keys
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        setZoom(cameraZoom * 1.05f);
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        setZoom(cameraZoom * 0.95f);
    }

    // Toggle trails with T key
    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        trailsEnabled = !trailsEnabled;
        tKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tKeyPressed = false;
    }
}

void Renderer::cleanup() {
    if (planetShaderProgram) {
        glDeleteProgram(planetShaderProgram);
        planetShaderProgram = 0;
    }
    if (trailShaderProgram) {
        glDeleteProgram(trailShaderProgram);
        trailShaderProgram = 0;
    }
    if (backgroundShaderProgram) {
        glDeleteProgram(backgroundShaderProgram);
        backgroundShaderProgram = 0;
    }
    if (planetVBO) {
        glDeleteBuffers(1, &planetVBO);
        planetVBO = 0;
    }
    if (trailVBO) {
        glDeleteBuffers(1, &trailVBO);
        trailVBO = 0;
    }
    if (backgroundVBO) {
        glDeleteBuffers(1, &backgroundVBO);
        backgroundVBO = 0;
    }
    if (planetVAO) {
        glDeleteVertexArrays(1, &planetVAO);
        planetVAO = 0;
    }
    if (trailVAO) {
        glDeleteVertexArrays(1, &trailVAO);
        trailVAO = 0;
    }
    if (backgroundVAO) {
        glDeleteVertexArrays(1, &backgroundVAO);
        backgroundVAO = 0;
    }
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}