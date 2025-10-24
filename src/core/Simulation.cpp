#include "planets/Simulation.hpp"
#include <iostream>
#include <random>

void Simulation::init() {
    planets.clear();
    // simple two-body starter (keeps compatibility)
    planets.emplace_back(Vector2(0.0f, 0.0f), Vector2(-0.5f, 0.0f)); // note: constructor order (v,p)
    planets.back().setMass(5.0f);

    planets.emplace_back(Vector2(0.0f, 0.0f), Vector2(0.5f, 0.0f));
    planets.back().setMass(5.0f);

    // clear any external physics engine registrations
    physics.clearBodies();
    for (auto &pl : planets) physics.addBody(&pl);
}

void Simulation::initRandom(int N, unsigned seed) {
    planets.clear();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> distPos(-2.5f, 2.5f);
    std::uniform_real_distribution<float> distVel(-0.03f, 0.03f);
    std::uniform_real_distribution<float> distMass(0.5f, 8.0f);
    std::uniform_real_distribution<float> distRad(0.02f, 0.08f);

    std::vector<glm::vec3> palette = {
        {0.95f, 0.85f, 0.30f},
        {0.50f, 0.80f, 1.00f},
        {0.90f, 0.40f, 0.40f},
        {0.80f, 0.65f, 0.95f},
        {0.60f, 0.90f, 0.60f},
        {0.95f, 0.75f, 0.60f}
    };

    planets.reserve(N);
    for (int i = 0; i < N; ++i) {
        Vector2 p(distPos(rng), distPos(rng));
        Vector2 v(distVel(rng), distVel(rng));
        float m = distMass(rng);
        float r = distRad(rng);
        Planet body(p, v, m, r);
        body.setColor(palette[i % palette.size()]);
        planets.push_back(body);
    }

    // clear physics engine registrations
    physics.clearBodies();
    for (auto &pl : planets) physics.addBody(&pl);
}

void Simulation::computeForces() {
    // compute pairwise gravitational forces with softening
    const size_t n = planets.size();
    temp_forces.clear();
    temp_forces.resize(n, Vector2(0.0f, 0.0f));

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            Vector2 diff = planets[j].getP() - planets[i].getP();
            float dist2 = diff.getX()*diff.getX() + diff.getY()*diff.getY() + softening*softening;
            float invDist = 1.0f / std::sqrt(dist2);
            float invDist3 = invDist * invDist * invDist;
            float scalar = G * planets[i].getMass() * planets[j].getMass() * invDist3;
            Vector2 f = diff * scalar;
            temp_forces[i] += f;
            temp_forces[j] -= f; // equal and opposite
        }
    }
}

void Simulation::integrate() {
    for (size_t i = 0; i < planets.size(); ++i) {
        planets[i].applyForce(temp_forces[i], deltaTime);
        planets[i].setP(planets[i].getP() + planets[i].getV() * deltaTime);
        planets[i].recordPosition();
    }
}

void Simulation::step() {
    if (planets.empty()) return;
    computeForces();
    integrate();
}

void Simulation::update() {
    // perform one fixed physics step
    step();
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
