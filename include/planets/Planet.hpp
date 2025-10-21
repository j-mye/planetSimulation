#ifndef PLANET_HPP
#define PLANET_HPP

#include <iostream>
#include <deque>
// #include <cmath>
#include "Vector2.hpp"


/**
 * @brief A Planet class representing a celestial body with position, velocity, speed,
 * and a trail of previous positions for visualization.
 */
class Planet {
private:
    // Current velocity and position
    Vector2 p;
    Vector2 v;
    float mass = 1.0f;

    // trail of previous positions for visualization
    std::deque<Vector2> trail;
    static const std::size_t maxTrailLength = 1000;

public:
    Planet() : v{0.0f, 0.0f}, p{0.0f, 0.0f} {}
    Planet(const Vector2& initialV) : v{initialV}, p{0.0f, 0.0f} {}
    Planet(const Vector2& initialV, const Vector2& initialP) : v{initialV}, p{initialP} {}

    float getMass() const { return mass; }
    void setMass(float m) { mass = m; }

    const Vector2& getV() const { return v; }
    void setV(const Vector2& newV) { v = newV; }

    const Vector2& getP() const { return p; }
    void setP(const Vector2& newP) { p = newP; }

    // Record current position into the trail 
    //  (call once per simulation frame)
    void recordPosition() {
        if (trail.size() >= maxTrailLength) trail.pop_front();
        trail.push_back(p);
    }

    // Integrate position using current velocity and change in time
    // void integrate(float dt) {
    //     p += v * dt;
    //     recordPosition();
    // }

    // accessors for trail and trail control
    const std::deque<Vector2>& getTrail() const { return trail; }
    void clearTrail() { trail.clear(); }

    // magnitude of current velocity
    float getSpeed() const {
        const float vx = v.getX();
        const float vy = v.getY();
        return std::sqrt(vx*vx + vy*vy);
    }

    // apply a force for duration dt (F = m * a => a = F/m)
    void applyForce(const Vector2& force, float dt) {
        // acceleration = force / mass
        Vector2 acc = force / static_cast<double>(mass);
        v += acc * dt;
    }

    // integrate position using current velocity and change in time
    void integrate(float dt) {
        p += v * dt;
        recordPosition();
    }

    // print planet info to console (debugging)
    void printInfo() const {
        std::cout << "Planet Position: (" << p.getX() << ", " << p.getY() << ")\n";
        std::cout << "Planet Velocity: (" << v.getX() << ", " << v.getY() << ")\n";
        std::cout << "Speed: " << getSpeed() << "  Trail length: " << trail.size() << "\n";
    }
};

#endif //PLANET_HPP