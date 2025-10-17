#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <iostream>
#include <cmath>
#include <glm/vec2.hpp>

class Vector2 {
private:
    glm::vec2 position, velocity;

public:
    Vector2() : position(0.0f, 0.0f), velocity(0.0f, 0.0f) {}
    Vector2(float x, float y) : position(x, y), velocity(0.0f, 0.0f) {}
    Vector2(float x, float y, float vx, float vy) : position(x, y), velocity(vx, vy) {}
    Vector2(const glm::vec2& pos, const glm::vec2& vel) : position(pos), velocity(vel) {}

    // compound assignments
    Vector2& operator+=(const Vector2& other) {
        position += other.position;
        velocity += other.velocity;
        return *this;
    }
    Vector2& operator-=(const Vector2& other) {
        position -= other.position;
        velocity -= other.velocity;
        return *this;
    }
    Vector2& operator*=(double scalar) {
        float s = static_cast<float>(scalar);
        position *= s;
        velocity *= s;
        return *this;
    }
    Vector2& operator/=(double scalar) {
        float s = static_cast<float>(scalar);
        position /= s;
        velocity /= s;
        return *this;
    }

    // binary operators
    Vector2 operator+(const Vector2& other) const {
        Vector2 tmp = *this;
        tmp += other;
        return tmp;
    }
    Vector2 operator-(const Vector2& other) const {
        Vector2 tmp = *this;
        tmp -= other;
        return tmp;
    }
    Vector2 operator*(double scalar) const {
        Vector2 tmp = *this;
        tmp *= scalar;
        return tmp;
    }
    Vector2 operator/(double scalar) const {
        Vector2 tmp = *this;
        tmp /= scalar;
        return tmp;
    }
};

#endif //VECTOR2_HPP