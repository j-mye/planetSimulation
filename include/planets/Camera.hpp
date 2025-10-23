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
    /**
     * @brief Constructor for the camera
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     */
    Camera(float screenWidth, float screenHeight);

    /**
     * @brief Update camera position based on center of mass of planets
     * @param planets Vector of Planet objects to track
     * @param deltaTime Time elapsed since last frame
     */
    void update(const std::vector<Planet>& planets, float deltaTime);

    /**
     * @brief Set the zoom level
     * @param z Zoom factor (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
     */
    void setZoom(float z);

    /**
     * @brief Zoom by a factor (multiplies current zoom)
     * @param factor Zoom factor to multiply current zoom by
     */
    void zoomBy(float factor);

    /**
     * @brief Pan the camera by an offset
     * @param dx X offset in world space
     * @param dy Y offset in world space
     */
    void pan(float dx, float dy);

    /**
     * @brief Get the current view matrix
     * @return 4x4 view matrix for OpenGL rendering
     */
    glm::mat4 getViewMatrix() const;

    /**
     * @brief Get the current camera position
     * @return Current camera position in world space
     */
    glm::vec2 getPosition() const { return position; }

    /**
     * @brief Get the current zoom level
     * @return Current zoom factor
     */
    float getZoom() const { return zoom; }

    /**
     * @brief Get the current target position (center of mass)
     * @return Current target position in world space
     */
    glm::vec2 getTarget() const { return target; }
    
    /**
     * @brief Enable or disable automatic zoom adjustment
     * @param enabled Whether automatic zoom should be active
     */
    void setAutoZoom(bool enabled) { autoZoom = enabled; }
    
    /**
     * @brief Check if automatic zoom is enabled
     * @return true if automatic zoom is enabled
     */
    bool isAutoZoomEnabled() const { return autoZoom; }

private:
    glm::vec2 position;   ///< Current camera position in world space
    glm::vec2 target;     ///< Target position (center of mass)
    float zoom;           ///< Zoom scale factor
    float smoothing;      ///< How quickly camera follows target (higher = faster)
    float aspect;         ///< Aspect ratio (width/height)
    bool autoZoom;        ///< Whether to automatically adjust zoom to fit all planets

    /**
     * @brief Compute the center of mass of all planets
     * @param planets Vector of Planet objects
     * @return Center of mass position in world space
     */
    glm::vec2 computeCenterOfMass(const std::vector<Planet>& planets) const;
    
    /**
     * @brief Compute the optimal zoom level to fit all planets within the window
     * @param planets Vector of Planet objects
     * @return Optimal zoom factor to fit all planets
     */
    float computeOptimalZoom(const std::vector<Planet>& planets) const;
};

#endif // CAMERA_HPP
