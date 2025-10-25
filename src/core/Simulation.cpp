#include "planets/Simulation.hpp"
#include <iostream>
#include <random>
#include <algorithm>

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
    // Collision handling using impulse-based mostly-elastic response with positional correction.
    // Collisions occur when distance <= sum of radii. We compute an impulse along the
    // collision normal and apply a restitution factor (collisionDamping) to simulate
    // mild energy loss. We also apply a position correction to resolve penetration.

    const float EPS = 1e-5f;
    for (size_t i = 0; i < planets.size(); ++i) {
        for (size_t j = i + 1; j < planets.size(); ++j) {
            Planet& A = planets[i];
            Planet& B = planets[j];

            Vector2 diff = B.getP() - A.getP();
            float dist = diff.length();
            float rA = A.getRadius();
            float rB = B.getRadius();
            float minDist = rA + rB;

            // If not overlapping, skip
            if (dist > minDist) continue;

            // Avoid division by zero
            if (dist < EPS) {
                // Nudge slightly along arbitrary axis
                diff = Vector2(EPS, 0.0f);
                dist = EPS;
            }

            // Normal from A to B
            Vector2 normal = diff / dist;

            // Relative velocity along normal
            Vector2 relVel = B.getV() - A.getV();
            float velAlongNormal = relVel.getX() * normal.getX() + relVel.getY() * normal.getY();

            // Compute restitution (use collisionDamping as restitution factor between 0..1)
            float e = std::clamp(collisionDamping, 0.0f, 1.0f);

            // Compute impulse scalar
            float mA = A.getMass();
            float mB = B.getMass();
            float invMassA = (mA > 0.0f) ? 1.0f / mA : 0.0f;
            float invMassB = (mB > 0.0f) ? 1.0f / mB : 0.0f;

            // Only apply impulse if bodies are moving towards each other or overlapping
            if (velAlongNormal < 0.0f) {
                float jImpulse = -(1.0f + e) * velAlongNormal / (invMassA + invMassB);
                Vector2 impulse = normal * jImpulse;

                // Apply velocity change
                A.setV(A.getV() - impulse * invMassA);
                B.setV(B.getV() + impulse * invMassB);
            }

            // Positional correction to resolve penetration (baumgarte-style)
            const float percent = 0.8f; // usually 20% to 80%
            const float slop = 0.01f;   // penetration allowance
            float penetration = std::max(minDist - dist - slop, 0.0f);
            if (penetration > 0.0f) {
                Vector2 correction = normal * (penetration / (invMassA + invMassB) * percent);
                A.setP(A.getP() - correction * invMassA);
                B.setP(B.getP() + correction * invMassB);
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
