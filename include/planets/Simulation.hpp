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

    // Collision physics
    float restitution = 0.8f; // coefficient of restitution (0=inelastic, 1=elastic)
    bool enableCollisions = true;
    bool enableMerging = true;
    float mergeVelocityThreshold = 0.05f; // merge if relative velocity below this

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
    void setGravityParams(float g, float eps) { G = g; softening = eps; }
    std::pair<float, float> getGravityParams() const { return {G, softening}; }

    // Collision controls
    void setRestitution(float r) { restitution = r; }
    float getRestitution() const { return restitution; }
    void setCollisionsEnabled(bool enabled) { enableCollisions = enabled; }
    bool areCollisionsEnabled() const { return enableCollisions; }
    void setMergingEnabled(bool enabled) { enableMerging = enabled; }
    bool isMergingEnabled() const { return enableMerging; }

    // (removed) bounds API
};

#endif //SIMULATION_HPP