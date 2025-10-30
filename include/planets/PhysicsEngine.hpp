#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <vector>
#include "Planet.hpp"

/**
 * @brief PhysicsEngine class to handle physics calculations for planets.
 * Uses the Newtonian gravity equations as the primary force model.
 */
class PhysicsEngine {
private:
    std::vector<Planet*> bodies;
    double G = 0.05;         // default sim-scale gravity (tunable)
    double softening = 0.02; // default softening (tunable)

public:
    PhysicsEngine() = default;
    void setGravityParams(float g, float eps) { G = g; softening = eps; }
    std::pair<float, float> getGravityParams() const { return { static_cast<float>(G), static_cast<float>(softening) }; }

    void addBody(Planet* body) { bodies.push_back(body); }
    void clearBodies() { bodies.clear(); }

    void computeForces(const float dt) {
        // Clear force accumulator for all bodies before computing forces
        for (Planet* p : bodies) {
            p->clearForces();
        }
        
        // Compute all gravitational forces and accumulate them
        const size_t n = bodies.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Planet* a = bodies[i];
                Planet* b = bodies[j];

                Vector2 r = b->getP() - a->getP();
                float dist = r.length();
                float denom = (dist*dist) + static_cast<float>(softening * softening);
                if (denom == 0.0f) continue;

                double forceMag = G * a->getMass() * b->getMass() / static_cast<double>(denom);
                Vector2 dir = r.normalized();
                Vector2 forceOnA = dir * forceMag;

                // Accumulate forces (dt not used in applyForce anymore)
                a->applyForce(forceOnA, dt);
                b->applyForce(-forceOnA, dt);
            }
        }
        
        // Apply accumulated forces to velocities
        for (Planet* p : bodies) {
            p->integrateVelocity(dt);
        }
    }

    void integrate(const float dt) {
        // Update positions using current velocities (semi-implicit Euler)
        for (Planet* p : bodies) {
            p->setP(p->getP() + (p->getV() * dt));
            p->recordPosition();
        }
    }
};

#endif //PHYSICS_ENGINE_HPP