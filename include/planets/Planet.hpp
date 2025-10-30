#ifndef PLANET_HPP
#define PLANET_HPP

#include <iostream>
#include <deque>
#include <glm/vec3.hpp>
#include "Vector2.hpp"


/**
 * @brief A Planet class representing a celestial body with position, velocity, speed,
 * and a trail of previous positions for visualization.
 */
class Planet {
private:
    Vector2 p;
    Vector2 v;
    Vector2 forceAccumulator; // Accumulated force for current timestep
    float mass = 1.0f;
    float radius = 1.0f;
    glm::vec3 color = glm::vec3(0.95f, 0.98f, 1.0f);

    // trail of previous positions for visualization
    std::deque<Vector2> trail;
    static const std::size_t maxTrailLength = 1000;

public:
    Planet() : v{0.0f, 0.0f}, p{0.0f, 0.0f}, forceAccumulator{0.0f, 0.0f} {}
    Planet(const Vector2& initialV) : v{initialV}, p{0.0f, 0.0f}, forceAccumulator{0.0f, 0.0f} {}
    Planet(const Vector2& initialV, const Vector2& initialP) : v{initialV}, p{initialP}, forceAccumulator{0.0f, 0.0f} {}
    Planet(const Vector2& initialP, const Vector2& initialV, float m, float r)
        : v{initialV}, p{initialP}, mass{m}, radius{r}, forceAccumulator{0.0f, 0.0f} {}
    
    float getMass() const { return mass; }
    void setMass(float m) { mass = m; }

    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }

    const Vector2& getV() const { return v; }
    void setV(const Vector2& newV) { v = newV; }

    const Vector2& getP() const { return p; }
    void setP(const Vector2& newP) { p = newP; }

    void recordPosition() {
        if (trail.size() >= maxTrailLength) trail.pop_front();
        trail.push_back(p);
    }

    const std::deque<Vector2>& getTrail() const { return trail; }
    void clearTrail() { trail.clear(); }

    float getSpeed() const {
        const float vx = v.getX();
        const float vy = v.getY();
        return std::sqrt(vx*vx + vy*vy);
    }

    void applyForce(const Vector2& force, float dt) {
        // Accumulate force (dt is not used here, integration happens separately)
        forceAccumulator += force;
    }
    
    void clearForces() {
        forceAccumulator = Vector2(0.0f, 0.0f);
    }
    
    void integrateVelocity(float dt) {
        // Apply accumulated forces to velocity: v += (F/m) * dt
        Vector2 acceleration = forceAccumulator / static_cast<double>(mass);
        v += acceleration * dt;
    }

    void printInfo() const {
        std::cout << "Planet Position: (" << p.getX() << ", " << p.getY() << ")\n";
        std::cout << "Planet Velocity: (" << v.getX() << ", " << v.getY() << ")\n";
        std::cout << "Speed: " << getSpeed() << "  Trail length: " << trail.size() << "\n";
    }

    const glm::vec3& getColor() const { return color; }
    void setColor(const glm::vec3& c) { color = c; }
};

#endif // PLANET_HPP