#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <cmath>
#include <glm/vec2.hpp>

/**
 * @brief Lightweight 2D vector wrapper around glm::vec2 with convenient ops.
 */
class Vector2 {
private:
    glm::vec2 vec;

public:
    Vector2() : vec(0.0f, 0.0f) {}
    Vector2(float x, float y) : vec{x, y} {}
    Vector2(const glm::vec2& v) : vec{v} {}

    // default destructor is fine (no manual delete)

    float getX() const { return vec.x; }
    float getY() const { return vec.y; }

    // compound assignments
    Vector2& operator+=(const Vector2& other) {
        vec += other.vec;
        return *this;
    }
    Vector2& operator-=(const Vector2& other) {
        vec -= other.vec;
        return *this;
    }
    Vector2& operator*=(double scalar) {
        float s = static_cast<float>(scalar);
        vec *= s;
        return *this;
    }
    Vector2& operator/=(double scalar) {
        float s = static_cast<float>(scalar);
        vec /= s;
        return *this;
    }

    // binary operators (implemented via compound ops)
    Vector2 operator+(const Vector2& other) const { Vector2 tmp = *this; tmp += other; return tmp; }
    Vector2 operator-(const Vector2& other) const { Vector2 tmp = *this; tmp -= other; return tmp; }
    Vector2 operator*(double scalar) const { Vector2 tmp = *this; tmp *= scalar; return tmp; }
    Vector2 operator/(double scalar) const { Vector2 tmp = *this; tmp /= scalar; return tmp; }

    // convenience: magnitude and normalization
    float length() const { return std::sqrt(vec.x * vec.x + vec.y * vec.y); }
    Vector2 normalized() const { float l = length(); return (l == 0.0f) ? Vector2(0,0) : (*this) / l; }

    // allow scalar * Vector2
    friend Vector2 operator*(double scalar, const Vector2& v) { return v * scalar; }
    // unary minus
    Vector2 operator-() const { return Vector2(-vec.x, -vec.y); }
};

#endif //VECTOR2_HPP