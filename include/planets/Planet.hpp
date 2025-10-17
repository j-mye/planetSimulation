#ifndef PLANET_HPP
#define PLANET_HPP

#include <iostream>
#include "Vector2.hpp"

class Planet {
private:
    Vector2 v = Vector2();

public:
    Planet() = default;
    Planet(const Vector2& initialV) : v(initialV) {}

    Vector2 getV() const {
        return v;
    }

    void setV(const Vector2& newV) {
        v = newV;
    }

    void printInfo() const {
        Vector2 pos = v;
        std::cout << "Planet Position: (" << pos.getX() << ", " << pos.getY() << ")\n";
        std::cout << "Planet Velocity: (" << pos.getVX() << ", " << pos.getVY() << ")\n";
    }
};

#endif //PLANET_HPP