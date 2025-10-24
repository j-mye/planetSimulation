#include "planets/Renderer.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

// Planet shader sources (simple, uniform-colored circular points)
static const char* planetVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in float aMass;
layout(location = 2) in vec3 aVelocity; // unused here, kept for stride compatibility
layout(location = 3) in vec3 aColor;

uniform mat4 uView;

out vec3 vColor;

void main() {
    gl_Position = uView * vec4(aPos, 0.0, 1.0);
    gl_PointSize = max(1.5, aMass * 2.0);
    vColor = aColor;
}
)";

static const char* planetFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 vColor;
void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.45, dist);
    FragColor = vec4(vColor, alpha);
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
    float alpha = (1.0 - vAge) * 0.6; // dim overall trail alpha to prevent bright center
    FragColor = vec4(uColor, alpha);
}
)";

// Background shader sources
static const char* backgroundVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vNdc;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vNdc = aPos; // NDC in [-1,1]
}
)";

static const char* backgroundFragmentShaderSrc = R"(
#version 330 core
in vec2 vNdc;
out vec4 FragColor;

uniform vec2 uCamPos;
uniform float uCamZoom;
uniform float uTime;

// Hash functions for procedural stars
float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
vec2 hash22(vec2 p) {
    float n = hash12(p);
    float m = hash12(p + 17.17);
    return vec2(n, m);
}

// World-anchored parallax starfield
float starLayer(vec2 world, float cellScale, float radius, float density, float twinkle) {
    // Scale world space into a star grid
    vec2 g = floor(world * cellScale);
    vec2 f = fract(world * cellScale);
    // Random star position within the cell
    vec2 starPos = hash22(g) - 0.5;
    // Distance to the star
    float d = length(f - (starPos + 0.5));
    // Star core falloff
    float core = smoothstep(radius, 0.0, d);
    // Random presence/brightness per cell
    float present = step(1.0 - density, hash12(g * 1.73));
    // Subtle twinkle
    float tw = 0.85 + 0.15 * sin(uTime * (1.0 + hash12(g * 3.1)) * 2.0);
    return core * present * (twinkle > 0.0 ? tw : 1.0);
}

void main() {
    // Convert NDC to world space using camera (parallax layers use different effective zooms)
    vec2 worldNear = uCamPos + (vNdc / max(uCamZoom, 0.001));        // near layer moves more
    vec2 worldFar  = uCamPos + (vNdc / max(uCamZoom * 3.0, 0.001));   // far layer moves less

    // Two star layers with different densities/scales
    float sFar  = starLayer(worldFar,  0.08, 0.05, 0.96, 1.0);  // many tiny distant stars
    float sNear = starLayer(worldNear, 0.035, 0.08, 0.90, 1.0); // fewer/larger nearby stars

    // Base space color gradient (subtle)
    vec3 top = vec3(0.02, 0.02, 0.07);
    vec3 bot = vec3(0.0,  0.0,  0.00);
    float y = (vNdc.y * 0.5 + 0.5);
    vec3 bg = mix(bot, top, y);

    // Star color (cool white with slight blue tint)
    vec3 starCol = vec3(0.85, 0.90, 1.0);
    vec3 col = bg + starCol * (0.6 * sFar + 1.0 * sNear);

    FragColor = vec4(col, 1.0);
}
)";

// Static starfield shader sources (NDC points, not camera-transformed)
static const char* starVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    gl_PointSize = 2.0; // constant small star size
    vColor = aColor;
}
)";

static const char* starFragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    float d = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.6, 0.0, d);
    FragColor = vec4(vColor, alpha);
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
                        starfieldEnabled(true),
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
    
    // Enable alpha blending (prevents bright additive hotspot at COM)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
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

    // Planet vertex attributes: position (vec2), mass (float), velocity (vec3), color (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

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

    // Initialize static starfield resources
    initStarfield();

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

void Renderer::drawBackground(const Camera& camera) {
    if (!starfieldEnabled) {
        return; // disabled for debugging visibility issues
    }
    // Draw static starfield (NDC points)
    (void)camera; // unused (static background)
    drawStarfield();
}

void Renderer::initStarfield() {
    starCount = 4000;
    std::vector<float> data;
    data.reserve(starCount * 5);

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dpos(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dcol(0.75f, 1.0f);
    std::uniform_int_distribution<int> huePick(0, 2);

    for (int i = 0; i < starCount; ++i) {
        float x = dpos(rng);
        float y = dpos(rng);
        int hue = huePick(rng);
        float r = dcol(rng), g = dcol(rng), b = dcol(rng);

        switch (hue) {
            case 0: // pale blue
                r *= 0.85f; g *= 0.92f; b *= 1.00f;
                break;
            case 1: // pale yellow
                r *= 1.00f; g *= 0.95f; b *= 0.85f;
                break;
            case 2: // neutral white
                r *= 0.95f; g *= 0.98f; b *= 1.00f;
            break;
        }

        data.push_back(x);
        data.push_back(y);
        data.push_back(r);
        data.push_back(g);
        data.push_back(b);
    }

    // Create VAO/VBO for stars
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);

    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Compile starfield shader program
    GLuint vs = compileShader(GL_VERTEX_SHADER, starVertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, starFragmentShaderSrc);
    starShaderProgram = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Renderer::drawStarfield() {
    if (starShaderProgram == 0 || starVAO == 0 || starCount <= 0) return;
    glUseProgram(starShaderProgram);
    glBindVertexArray(starVAO);
    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthEnabled) glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_POINTS, 0, starCount);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::drawPlanets(const std::vector<Planet>& planets, const Camera& camera) {
    if (planets.empty()) return;

    std::vector<float> planetData;
    planetData.reserve(planets.size() * 9); 
    for (const auto& planet : planets) {
        planetData.push_back(planet.getP().getX());
        planetData.push_back(planet.getP().getY());
        planetData.push_back(planet.getMass());
        planetData.push_back(planet.getV().getX());
        planetData.push_back(planet.getV().getY());
        planetData.push_back(0.0f); // Z component (not used in 2D)
        const glm::vec3 c = planet.getColor();
        planetData.push_back(c.r);
        planetData.push_back(c.g);
        planetData.push_back(c.b);
    }

    // Upload planet data to GPU
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
    if (trails.size() != planets.size()) {
        trails.resize(planets.size());
    }

    for (size_t i = 0; i < planets.size(); ++i) {
        trails[i].push_back(planets[i].getP());
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

    for (size_t i = 0; i < trails.size() && i < planets.size(); ++i) {
        if (trails[i].size() < 2) continue;

        std::vector<float> trailData;
        trailData.reserve(trails[i].size() * 3);

        float maxAge = static_cast<float>(trails[i].size() - 1);
        for (size_t j = 0; j < trails[i].size(); ++j) {
            trailData.push_back(trails[i][j].getX());
            trailData.push_back(trails[i][j].getY());
            trailData.push_back(j / maxAge);
        }

        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
        glBufferData(GL_ARRAY_BUFFER, trailData.size() * sizeof(float), trailData.data(), GL_DYNAMIC_DRAW);

        const glm::vec3 c = planets[i].getColor();
        glUniform3f(trailLoc_uColor, c.r, c.g, c.b);

        glBindVertexArray(trailVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(trails[i].size()));
    }

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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

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

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        setZoom(cameraZoom * 1.05f);
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        setZoom(cameraZoom * 0.95f);
    }

    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        trailsEnabled = !trailsEnabled;
        tKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tKeyPressed = false;
    }
}

void Renderer::cleanup() {
    if (starShaderProgram) {
        glDeleteProgram(starShaderProgram);
        starShaderProgram = 0;
    }
    if (starVBO) {
        glDeleteBuffers(1, &starVBO);
        starVBO = 0;
    }
    if (starVAO) {
        glDeleteVertexArrays(1, &starVAO);
        starVAO = 0;
    }
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