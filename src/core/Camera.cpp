#include "planets/Camera.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera::Camera(float screenWidth, float screenHeight)
    : position(0.0f), target(0.0f), zoom(1.0f), smoothing(5.0f),
    aspect(screenWidth / screenHeight), autoZoom(true) {}

void Camera::update(const std::vector<Planet>& planets, float deltaTime) {
    if (planets.empty()) return;

    target = computeCenterOfMass(planets);
    if (!initialized) {
        // Snap on first frame to avoid flash or overshoot
        position = target;
        if (autoZoom) {
            zoom = computeOptimalZoom(planets);
        }
        initialized = true;
        return;
    }
    const float lerpFactor = 1.0f - std::exp(-smoothing * deltaTime);

    // Smoothly interpolate camera position toward target
    position += (target - position) * lerpFactor;
    
    // Auto-adjust zoom to fit all planets within the window (if enabled)
    if (autoZoom) {
        float optimalZoom = computeOptimalZoom(planets);
        const float zoomLerpFactor = 1.0f - std::exp(-smoothing * deltaTime * 0.5f); // Slower zoom adjustment
        zoom += (optimalZoom - zoom) * zoomLerpFactor;
    }
}

void Camera::setZoom(float z) {
    // Allow a much wider range to accommodate large/small scenes
    zoom = std::clamp(z, 0.0005f, 100.0f);
}

void Camera::zoomBy(float factor) {
    setZoom(zoom * factor);
}

void Camera::pan(float dx, float dy) {
    position += glm::vec2(dx, dy);
}

glm::mat4 Camera::getViewMatrix() const {
    glm::mat4 view(1.0f);
    // Scale then translate: view = S * T(-position)
    // Ensures recentering remains correct as zoom changes
    view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
    view = glm::translate(view, glm::vec3(-position, 0.0f));
    return view;
}


glm::vec2 Camera::computeCenterOfMass(const std::vector<Planet>& planets) const {
    if (planets.empty()) return glm::vec2(0.0f);

    glm::dvec2 weightedSum(0.0);
    double totalMass = 0.0;

    for (const auto& planet : planets) {
        const double mass = static_cast<double>(planet.getMass());
        if (mass <= 0.0) continue;

        const glm::dvec2 pos(
            static_cast<double>(planet.getP().getX()),
            static_cast<double>(planet.getP().getY())
        );

        weightedSum += pos * mass;
        totalMass += mass;
    }

    if (totalMass == 0.0) return glm::vec2(0.0f);

    const glm::dvec2 com = weightedSum / totalMass;
    return glm::vec2(com);
}

float Camera::computeOptimalZoom(const std::vector<Planet>& planets) const {
    if (planets.empty()) return 1.0f;
    
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& planet : planets) {
        float x = planet.getP().getX();
        float y = planet.getP().getY();
        float radius = planet.getRadius();
        
        // Expand bounding box by planet radius
        minX = std::min(minX, x - radius);
        maxX = std::max(maxX, x + radius);
        minY = std::min(minY, y - radius);
        maxY = std::max(maxY, y + radius);
    }
    
    // Calculate the size of the bounding box
    float width = maxX - minX;
    float height = maxY - minY;
    
    // Add 20% padding
    width *= 1.2f;
    height *= 1.2f;
    
    // Guard against degenerate sizes to avoid INF zoom
    width = std::max(width, 1e-4f);
    height = std::max(height, 1e-4f);

    // Calculate zoom needed to fit the bounding box in the window
    // The window coordinates go from -1 to 1, so total size is 2
    float zoomX = 2.0f / width;
    float zoomY = 2.0f / height;
    
    // Use the smaller zoom to ensure everything fits
    float optimalZoom = std::min(zoomX, zoomY);
    
    // Clamp to reasonable bounds
    return std::clamp(optimalZoom, 0.0005f, 100.0f);
}

void Camera::reset() {
    position = glm::vec2(0.0f, 0.0f);
    target = glm::vec2(0.0f, 0.0f);
    zoom = 1.0f;
    initialized = false; // Allow snap on next update
}
