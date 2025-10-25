#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <vector>
#include "PhysicsEngine.hpp"
#include "Planet.hpp"

class Simulation {
private:
    PhysicsEngine physics;
    std::vector<Planet> planets;
    float deltaTime = 0.0015f;
    bool running = false;

    // N-body physics parameters (softened gravity)
    float G = 0.05f;
    float softening = 0.02f;

    // Soft collision physics (repulsive force-based)
    float collisionStrength = 0.5f; // strength of repulsive force
    float collisionDamping = 0.95f; // velocity damping during collision (0.9-0.99)
    bool enableCollisions = true;

    // Internal helpers
    void computeForces();
    void integrate();
    void handleCollisions();
    std::vector<Vector2> temp_forces;

public:
    Simulation() = default;
    void init();
    // Initialize random system with N bodies
    void initRandom(int N, unsigned seed = 1337);

    // Advance physics by internal deltaTime (one fixed step)
    void step();

    // General update hook (kept for compatibility)
    void update();

    void run(int steps = 1000);
    void printStatus() const;
    std::vector<Planet>& getPlanets() { return planets; }
    void setTimeStep(float dt) { deltaTime = dt; }
    float getTimeStep() const { return deltaTime; }
    void setGravityParams(float g, float eps) { G = g; softening = eps; }
    std::pair<float, float> getGravityParams() const { return {G, softening}; }

    // Collision controls
    void setCollisionStrength(float strength) { collisionStrength = strength; }
    float getCollisionStrength() const { return collisionStrength; }
    void setCollisionDamping(float damping) { collisionDamping = damping; }
    float getCollisionDamping() const { return collisionDamping; }
    void setCollisionsEnabled(bool enabled) { enableCollisions = enabled; }
    bool areCollisionsEnabled() const { return enableCollisions; }

    // (removed) bounds API
};

#endif //SIMULATION_HPP