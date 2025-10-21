#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <vector>
#include "PhysicsEngine.hpp"
#include "Planet.hpp"

class Simulation {
private:
    PhysicsEngine physics;
    std::vector<Planet> planets;
    float deltaTime = 0.016f;
    bool running = false;

public:
    Simulation() = default;
    void init();
    void update();
    void run(int steps = 1000);
    void printStatus() const;
    std::vector<Planet>& getPlanets() { return planets; }
};

#endif //SIMULATION_HPP