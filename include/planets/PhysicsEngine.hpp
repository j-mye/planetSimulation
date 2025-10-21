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
    double G = 6.67430e-11; // gravitational constant (can be scaled)
    double softening = 1e-3;

public:
    PhysicsEngine() = default;
    void setG(double g) { G = g; }
    void setSoftening(double s) { softening = s; }

    void addBody(Planet* body) { bodies.push_back(body); }
    void clearBodies() { bodies.clear(); }

    void computeForces(float dt) {
        // compute pairwise forces and apply accelerations to bodies for timestep dt
        const size_t n = bodies.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Planet* a = bodies[i];
                Planet* b = bodies[j];

                Vector2 r = b->getP() - a->getP();
                float dist = r.length();
                float denom = (dist*dist) + static_cast<float>(softening);
                if (denom == 0.0f) continue;

                double forceMag = G * a->getMass() * b->getMass() / static_cast<double>(denom);
                Vector2 dir = r.normalized();
                Vector2 forceOnA = dir * forceMag;

                a->applyForce(forceOnA, dt);
                b->applyForce(-forceOnA, dt);
            }
        }
    }

    void integrate(float dt) {
        for (Planet* p : bodies) p->integrate(dt);
    }
};

#endif //PHYSICS_ENGINE_HPP