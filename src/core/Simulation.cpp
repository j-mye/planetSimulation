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

void Simulation::handleCollisions() {
    if (!enableCollisions || planets.size() < 2) return;

    std::vector<bool> merged(planets.size(), false);

    for (size_t i = 0; i < planets.size(); ++i) {
        if (merged[i]) continue;

        for (size_t j = i + 1; j < planets.size(); ++j) {
            if (merged[j]) continue;

            Planet& a = planets[i];
            Planet& b = planets[j];

            // Check collision: distance < sum of radii
            Vector2 diff = b.getP() - a.getP();
            float dist = diff.length();
            float radSum = a.getRadius() + b.getRadius();

            if (dist < radSum && dist > 1e-6f) {
                // Collision detected
                Vector2 normal = diff.normalized();
                Vector2 relVel = b.getV() - a.getV();
                float relVelMag = relVel.length();

                // Check if merging conditions are met
                if (enableMerging && relVelMag < mergeVelocityThreshold) {
                    // Merge: conserve momentum and mass
                    float m1 = a.getMass();
                    float m2 = b.getMass();
                    float totalMass = m1 + m2;

                    // New velocity (momentum conservation)
                    Vector2 newVel = (a.getV() * m1 + b.getV() * m2) / totalMass;

                    // New position (center of mass)
                    Vector2 newPos = (a.getP() * m1 + b.getP() * m2) / totalMass;

                    // New radius (volume conservation: V = 4/3 π r³)
                    float r1 = a.getRadius();
                    float r2 = b.getRadius();
                    float newRadius = std::pow(r1*r1*r1 + r2*r2*r2, 1.0f/3.0f);

                    // Blend colors by mass
                    glm::vec3 c1 = a.getColor();
                    glm::vec3 c2 = b.getColor();
                    glm::vec3 newColor = (c1 * m1 + c2 * m2) / totalMass;

                    // Update planet a with merged properties
                    a.setMass(totalMass);
                    a.setRadius(newRadius);
                    a.setP(newPos);
                    a.setV(newVel);
                    a.setColor(newColor);

                    // Mark planet b for removal
                    merged[j] = true;
                } else {
                    // Elastic/inelastic collision response
                    // Relative velocity along collision normal
                    float vn = (relVel.getX() * normal.getX() + relVel.getY() * normal.getY());

                    // Don't resolve if moving apart
                    if (vn > 0.0f) continue;

                    // Impulse magnitude using coefficient of restitution
                    float m1 = a.getMass();
                    float m2 = b.getMass();
                    float impulse = -(1.0f + restitution) * vn / (1.0f/m1 + 1.0f/m2);

                    // Apply impulse
                    Vector2 impulseVec = normal * impulse;
                    a.setV(a.getV() - impulseVec / m1);
                    b.setV(b.getV() + impulseVec / m2);

                    // Separate bodies to prevent overlap
                    float overlap = radSum - dist;
                    float separationA = overlap * (m2 / (m1 + m2));
                    float separationB = overlap * (m1 / (m1 + m2));
                    a.setP(a.getP() - normal * separationA);
                    b.setP(b.getP() + normal * separationB);
                }
            }
        }
    }

    // Remove merged planets
    if (enableMerging) {
        for (int i = static_cast<int>(planets.size()) - 1; i >= 0; --i) {
            if (merged[i]) {
                planets.erase(planets.begin() + i);
            }
        }
    }
}

void Simulation::step() {
    if (planets.empty()) return;
    computeForces();
    integrate();
    handleCollisions();
}

void Simulation::update() {
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
