#include "planets/Camera.hpp"
#include <algorithm>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera::Camera(float screenWidth, float screenHeight)
    : position(0.0f), target(0.0f), zoom(1.0f), smoothing(5.0f),
    aspect(screenWidth / screenHeight) {}

void Camera::update(const std::vector<Planet>& planets, float deltaTime) {
    if (planets.empty()) return;

    // Check if we're following a specific planet
    if (followedPlanetIndex >= 0 && followedPlanetIndex < static_cast<int>(planets.size())) {
        // Follow the specific planet
        const Planet& followedPlanet = planets[followedPlanetIndex];
        target = glm::vec2(followedPlanet.getP().getX(), followedPlanet.getP().getY());
    } else {
        // Follow COM (default behavior)
        followedPlanetIndex = -1; // reset if out of bounds
        
        // Compute inlier set by excluding far-out bodies
        std::vector<size_t> inliers;
        computeInliers(planets, inliers);

        // Recompute COM using inliers for better stability
        if (!inliers.empty()) {
            glm::dvec2 weightedSum(0.0);
            double totalMass = 0.0;
            for (size_t idx : inliers) {
                const Planet& pl = planets[idx];
                const double mass = static_cast<double>(pl.getMass());
                const glm::dvec2 pos(
                    static_cast<double>(pl.getP().getX()),
                    static_cast<double>(pl.getP().getY())
                );
                weightedSum += pos * mass;
                totalMass += mass;
            }
            target = (totalMass > 0.0) ? glm::vec2(weightedSum / totalMass) : computeCenterOfMass(planets);
        } else {
            target = computeCenterOfMass(planets);
        }
    }
    if (!initialized) {
        // Snap on first frame to avoid flash or overshoot
        position = target;
        zoom = computeOptimalZoom(planets) * zoomOffset;
        initialized = true;
        return;
    }
    const float lerpFactor = 1.0f - std::exp(-smoothing * deltaTime);

    // Smoothly interpolate camera position toward target
    position += (target - position) * lerpFactor;
    
    // Auto-adjust zoom to fit all planets within the window (always on)
    // Apply user's zoom offset on top of optimal zoom
    float optimalZoom = computeOptimalZoom(planets);
    float targetZoom = optimalZoom * zoomOffset;
    const float zoomLerpFactor = 1.0f - std::exp(-smoothing * deltaTime * 0.5f); // Slower zoom adjustment
    zoom += (targetZoom - zoom) * zoomLerpFactor;
}

void Camera::setZoom(float z) {
    // When user manually sets zoom, calculate the offset from current optimal zoom
    // This preserves the user's zoom preference relative to auto-zoom
    zoom = std::clamp(z, 0.0005f, 100.0f);
}

void Camera::zoomBy(float factor) {
    // Apply factor to zoom offset to preserve user's zoom preference
    zoomOffset *= factor;
    zoomOffset = std::clamp(zoomOffset, 0.1f, 10.0f); // Keep offset in reasonable range
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
    // Build inlier set and compute AABB over inliers
    std::vector<size_t> inliers;
    computeInliers(planets, inliers);
    const bool useAll = inliers.empty();

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    
    if (useAll) {
        for (const auto& planet : planets) {
            float x = planet.getP().getX();
            float y = planet.getP().getY();
            float radius = planet.getRadius();
            minX = std::min(minX, x - radius);
            maxX = std::max(maxX, x + radius);
            minY = std::min(minY, y - radius);
            maxY = std::max(maxY, y + radius);
        }
    } else {
        for (size_t idx : inliers) {
            const auto& planet = planets[idx];
            float x = planet.getP().getX();
            float y = planet.getP().getY();
            float radius = planet.getRadius();
            minX = std::min(minX, x - radius);
            maxX = std::max(maxX, x + radius);
            minY = std::min(minY, y - radius);
            maxY = std::max(maxY, y + radius);
        }
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

void Camera::computeInliers(const std::vector<Planet>& planets, std::vector<size_t>& indices) const {
    indices.clear();
    if (planets.empty()) return;

    // Preliminary COM using all bodies (mass-weighted)
    glm::dvec2 weightedSum(0.0);
    double totalMass = 0.0;
    for (const auto& p : planets) {
        double m = (double)p.getMass();
        glm::dvec2 pos((double)p.getP().getX(), (double)p.getP().getY());
        weightedSum += pos * m; totalMass += m;
    }
    glm::vec2 com = (totalMass > 0.0) ? glm::vec2(weightedSum / totalMass) : glm::vec2(0.0f);

    // Compute distances to COM
    std::vector<float> dists; dists.reserve(planets.size());
    for (const auto& p : planets) {
        glm::vec2 pos(p.getP().getX(), p.getP().getY());
        dists.push_back(glm::length(pos - com));
    }

    // Median distance
    std::vector<float> sorted = dists;
    if (sorted.empty()) return;
    size_t mid = sorted.size() / 2;
    std::nth_element(sorted.begin(), sorted.begin() + mid, sorted.end());
    float median = sorted[mid];
    if (median <= 0.0f) median = 1e-4f;

    float threshold = outlierMultiplier * median;
    for (size_t i = 0; i < planets.size(); ++i) {
        if (dists[i] <= threshold) indices.push_back(i);
    }
}

void Camera::reset() {
    position = glm::vec2(0.0f, 0.0f);
    target = glm::vec2(0.0f, 0.0f);
    zoom = 1.0f;
    zoomOffset = 1.0f; // Reset zoom offset
    followedPlanetIndex = -1; // Reset to COM follow
    initialized = false; // Allow snap on next update
}
