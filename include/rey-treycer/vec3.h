#pragma once
#include <math.h>

class Vec3 {
public:
    float x = 0;
    float y = 0;
    float z = 0;

    Vec3(float a, float b, float c) {
        x = a;
        y = b;
        z = c;
    }

    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }
    Vec3& operator+=(const Vec3 &v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    Vec3& operator-=(const Vec3 &v) {
        return *this += -v;
    }
    Vec3 operator*=(const float t) {
        x *= t;
        y *= t;
        z *= t;
        return *this;
    }
    Vec3 operator/=(const float t) {
        return *this *= 1/t;
    }
    bool operator==(const Vec3 &v) {
        Vec3 u = *this;
        return u.x == v.x and u.y == v.y and u.z == v.z;
    }
    bool operator!=(const Vec3 &v) {
        return !(*this == v);
    }
    float dot(Vec3 v) {
        return x * v.x + y * v.y + z * v.z;
    }
    float squared_length() {
        return dot(*this);
    }
    float length() {
        return sqrt(dot(*this));
    }
    Vec3 cross(Vec3 v) {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
    Vec3 normalize() {
        const float l = length();
        return Vec3(x / l, y / l, z / l);
    }
};

inline Vec3 operator+(const Vec3 &u, const Vec3 &v) {
    return Vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}
inline Vec3 operator-(const Vec3 &u, const Vec3 &v) {
    return u + (-v);
}
inline Vec3 operator*(const Vec3 &u, const Vec3 &v) {
    return Vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}
inline Vec3 operator*(const Vec3 &u, const float t) {
    return Vec3(u.x * t, u.y * t, u.z * t);
}
inline Vec3 operator*(const float t, const Vec3 &u) {
    return u * t;
}
inline Vec3 operator/(const Vec3 v, const float t) {
    return v * (1/t);
}
inline Vec3 operator/(const float t, const Vec3 v) {
    return Vec3(t/v.x, t/v.y, t/v.z);
}
inline Vec3 operator/(const Vec3 u, const Vec3 v) {
    return u * (1/v);
}
