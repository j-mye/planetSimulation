#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <iostream>
#include <cmath>
#include <glm/vec2.hpp>

/**
 * @brief A simple 2D vector class that defines basic vector operations and
 * memory management. (uses glm::vec2)
 */
class Vector2 {
private:
    glm::vec2 vec;

public:
    Vector2() : vec(0.0f, 0.0f) {}
    Vector2(float x, float y) : vec{x, y} {}
    Vector2(const glm::vec2& vec) : vec{vec} {}
    ~Vector2() {
        delete &vec;
    }

    float getX() const { return vec.x; }
    float getY() const { return vec.y; }

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