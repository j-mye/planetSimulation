#include "planets/Simulation.hpp"
#include <iostream>

void Simulation::init() {
    planets.clear();
    planets.emplace_back(Vector2(0.0f, 0.0f), Vector2(-0.5f, 0.0f)); // note: constructor order (v,p)
    planets.back().setMass(5.0f);

    planets.emplace_back(Vector2(0.0f, 0.0f), Vector2(0.5f, 0.0f));
    planets.back().setMass(5.0f);

    // register with physics
    physics.clearBodies();
    for (auto &pl : planets) physics.addBody(&pl);
}

void Simulation::update() {
    physics.computeForces(deltaTime);
    physics.integrate(deltaTime);
}

void Simulation::run(int steps) {
    running = true;
}

void Simulation::printStatus() const {
    std::cout << "Simulation status:\n";
    int idx = 0;
    for (const auto &pl : planets) {
        std::cout << "Planet " << idx++ << ": ";
        pl.printInfo();
    }
}
