#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <vector>
#include "PhysicsEngine.hpp"
#include "Planet.hpp"

/**
 * Simulation class managing a system of planets with N-body physics.
 */
class Simulation {
private:
    PhysicsEngine physics;
    std::vector<Planet> planets;
    float deltaTime = 0.0015f;

public:
    Simulation() = default;
    void init();
    void initRandom(int N, unsigned seed = 1337);
    void step();
    void update();
    
    std::vector<Planet>& getPlanets() { return planets; }
    void setTimeStep(float dt) { deltaTime = dt; }
    float getTimeStep() const { return deltaTime; }
    void setGravityParams(float g, float eps) { physics.setGravityParams(g, eps); }
    std::pair<float, float> getGravityParams() const { return physics.getGravityParams(); }
};

#endif //SIMULATION_HPP