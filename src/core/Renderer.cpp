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
layout(location = 4) in float aRadius;

uniform mat4 uView;
uniform float uPixelPerWorld; // pixels per world unit (framebuffer-space)
uniform float uRadiusScale;   // visual amplification of radius

out vec3 vColor;

void main() {
    gl_Position = uView * vec4(aPos, 0.0, 1.0);

    // Convert world-space radius to pixels using uPixelPerWorld; ensure a minimum size
    const float MIN_POINT_SIZE = 2.5;
    float ps = aRadius * uRadiusScale * 3.0;
    gl_PointSize = max(MIN_POINT_SIZE, ps);

    vColor = aColor;
}
)";

static const char* planetFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 vColor;
void main() {
    // Use squared distance to avoid an expensive sqrt (length)
    vec2 d = gl_PointCoord - vec2(0.5);
    float dist2 = dot(d, d);
    // Smoothstep on squared radii: inner radius ~0.45, outer radius ~0.5
    float alpha = 1.0 - smoothstep(0.45 * 0.45, 0.5 * 0.5, dist2);
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
    // Linear fade based on normalized age. Clamp to avoid numerical issues.
    float a = clamp(1.0 - vAge, 0.0, 1.0);
    float alpha = a * 0.6;
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
// Efficient hash (cheap and decent distribution) and lightweight star layer
float hash12(vec2 p) {
    // sin-based hash is cheap and works well for procedural noise in GLSL
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
vec2 hash22(vec2 p) {
    return vec2(hash12(p), hash12(p + vec2(41.23, 17.17)));
}

float starLayer(vec2 world, float cellScale, float radius, float density, float twinkle) {
    vec2 g = floor(world * cellScale);
    vec2 f = fract(world * cellScale);
    vec2 starPos = hash22(g) - 0.5;
    float d2 = dot(f - (starPos + 0.5), f - (starPos + 0.5));
    float core = 1.0 - smoothstep(radius * radius * 0.6, radius * radius, d2);
    float present = step(1.0 - density, hash12(g * 1.73));
    float tw = 0.9 + 0.1 * sin(uTime * (1.0 + hash12(g * 3.1)));
    return core * present * (twinkle > 0.0 ? tw : 1.0);
}

void main() {
    vec2 worldNear = uCamPos + (vNdc / max(uCamZoom, 0.001));
    vec2 worldFar  = uCamPos + (vNdc / max(uCamZoom * 3.0, 0.001));

    float sFar  = starLayer(worldFar,  0.08, 0.05, 0.96, 1.0);
    float sNear = starLayer(worldNear, 0.035, 0.08, 0.90, 1.0);

    vec3 top = vec3(0.02, 0.02, 0.07);
    vec3 bot = vec3(0.0,  0.0,  0.00);
    float y = vNdc.y * 0.5 + 0.5;
    vec3 bg = mix(bot, top, y);

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
    vec2 c = gl_PointCoord - vec2(0.5);
    float d2 = dot(c, c);
    // Smooth falloff using squared distances for better performance
    float alpha = 1.0 - smoothstep(0.0, 0.6 * 0.6, d2);
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

Renderer::Renderer(int w, int h, const char* title)
        : width(w), height(h), window(nullptr), 
            planetVAO(0), planetVBO(0), planetShaderProgram(0),
            trailVAO(0), trailVBO(0), trailShaderProgram(0),
            backgroundVAO(0), backgroundVBO(0), backgroundShaderProgram(0),
            cameraPosition(0.0f, 0.0f), cameraZoom(1.0f),
            trailsEnabled(true), maxTrailLength(500),
            starfieldEnabled(true) {
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
    loc_uPixelPerWorld = glGetUniformLocation(planetShaderProgram, "uPixelPerWorld");
    loc_uRadiusScale = glGetUniformLocation(planetShaderProgram, "uRadiusScale");

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

    // Planet vertex attributes: position (vec2), mass (float), velocity (vec3), color (vec3), radius (float)
    // Layout: pos(2), mass(1), vel(3), color(3), radius(1) = 10 floats
    GLsizei pstride = 10 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, pstride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, pstride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, pstride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, pstride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, pstride, (void*)(9 * sizeof(float)));

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

    updateViewMatrix();
    initStarfield();

    return true;
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
        return;
    }
    (void)camera;
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
    planetData.reserve(planets.size() * 10);  // 10 values per planet (2 position, 1 mass, 3 velocity, 3 color, 1 radius)

    for (const auto& planet : planets) {
        planetData.push_back(planet.getP().getX());
        planetData.push_back(planet.getP().getY());
        planetData.push_back(planet.getMass());
        planetData.push_back(planet.getV().getX());
        planetData.push_back(planet.getV().getY());
        planetData.push_back(0.0f); // Z component (unused in 2D)
        const glm::vec3 c = planet.getColor();
        planetData.push_back(c.r);
        planetData.push_back(c.g);
        planetData.push_back(c.b);
        planetData.push_back(planet.getRadius());
    }

    // Upload planet data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, planetData.size() * sizeof(float), planetData.data(), GL_DYNAMIC_DRAW);

    glUseProgram(planetShaderProgram);
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glUniformMatrix4fv(loc_uView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    
    int fbW = 0, fbH = 0; glfwGetFramebufferSize(window, &fbW, &fbH);
    float pixelPerWorld = camera.getZoom() * static_cast<float>(fbH);
    if (loc_uPixelPerWorld >= 0) glUniform1f(loc_uPixelPerWorld, pixelPerWorld);
    if (loc_uRadiusScale   >= 0) glUniform1f(loc_uRadiusScale, planetRadiusScale);

    glBindVertexArray(planetVAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(planets.size()));

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
    switch (GLFW_PRESS) {
        case GLFW_KEY_LEFT:
            pan(-panSpeed, 0.0f);
            break;
        case GLFW_KEY_A:
            pan(-panSpeed, 0.0f);
            break;
        case GLFW_KEY_RIGHT:
            pan(panSpeed, 0.0f);
            break;
        case GLFW_KEY_D:
            pan(panSpeed, 0.0f);
            break;
        case GLFW_KEY_UP:
            pan(0.0f, panSpeed);
            break;
        case GLFW_KEY_W:
            pan(0.0f, panSpeed);
            break;
        case GLFW_KEY_DOWN:
            pan(0.0f, -panSpeed);
            break;
        case GLFW_KEY_S:
            pan(0.0f, -panSpeed);
            break;
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