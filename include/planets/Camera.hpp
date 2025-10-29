#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Planet.hpp"

/**
 * @brief Camera class that automatically follows the center of mass (COM) of the planetary system
 * 
 * This camera system provides smooth tracking of the center of mass while allowing
 * manual zoom and pan controls for exploration. The camera smoothly interpolates
 * toward the COM to avoid jitter and provide cinematic visuals.
 */
class Camera {
public:
    Camera(float screenWidth, float screenHeight);

    void update(const std::vector<Planet>& planets, float deltaTime);
    void setZoom(float z);
    void zoomBy(float factor);
    void pan(float dx, float dy);
    glm::mat4 getViewMatrix() const;
    glm::vec2 getPosition() const { return position; }
    float getZoom() const { return zoom; }
    glm::vec2 getTarget() const { return target; }
    void reset();

    // Planet following
    void setFollowedPlanet(int index) { followedPlanetIndex = index; }
    int getFollowedPlanet() const { return followedPlanetIndex; }
    bool isFollowingPlanet() const { return followedPlanetIndex >= 0; }

    // Auto-zoom outlier exclusion controls
    void setOutlierMultiplier(float m) { outlierMultiplier = std::max(1.0f, m); }
    float getOutlierMultiplier() const { return outlierMultiplier; }

private:
    glm::vec2 position, target;
    float zoom;
    float zoomOffset = 1.0f; // User's manual zoom multiplier (1.0 = no offset)
    float smoothing;
    float aspect;
    bool initialized = false; // snap on first update
    int followedPlanetIndex = -1; // -1 = follow COM, >= 0 = follow specific planet
    float outlierMultiplier = 3.0f; // Exclude bodies farther than m * median distance

    glm::vec2 computeCenterOfMass(const std::vector<Planet>& planets) const;
    float computeOptimalZoom(const std::vector<Planet>& planets) const;
    void computeInliers(const std::vector<Planet>& planets, std::vector<size_t>& indices) const;
};

#endif // CAMERA_HPP
