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
    void setAutoZoom(bool enabled) { autoZoom = enabled; }
    bool isAutoZoomEnabled() const { return autoZoom; }
    void reset(); // Reset to initial state

private:
    glm::vec2 position, target;
    float zoom;
    float smoothing;
    float aspect;
    bool autoZoom;
    bool initialized = false; // snap on first update

    glm::vec2 computeCenterOfMass(const std::vector<Planet>& planets) const;
    float computeOptimalZoom(const std::vector<Planet>& planets) const;
};

#endif // CAMERA_HPP
