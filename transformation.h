#pragma once
#include "constant.h"

inline Vec3 _rotate(Vec3 v, float x, float y, float z) {
    // rotation matrix
    // [ a b c
    //   d e f
    //   g h i ]
    float a = cos(y) * cos(z);
    float b = sin(x) * sin(y) * cos(z) - cos(x) * sin(z);
    float c = cos(x) * sin(y) * cos(z) + sin(x) * sin(z);
    float d = cos(y) * sin(z);
    float e = sin(x) * sin(y) * sin(z) + cos(x) * cos(z);
    float f = cos(x) * sin(y) * sin(z) - sin(x) * cos(z);
    float g = -sin(y);
    float h = sin(x) * cos(y);
    float i = cos(x) * cos(y);
    // multiply matrix
    Vec3 u = VEC3_ZERO;
    u.x = a * v.x + b * v.y + c * v.z;
    u.y = d * v.x + e * v.y + f * v.z;
    u.z = g * v.x + h * v.y + i * v.z;
    return u;
}
inline Vec3 _rotate(Vec3 v, Vec3 a) {
    return _rotate(v, a.x, a.y, a.z);
}
inline Vec3 _rotate_x(Vec3 v, float a) {
    Vec3 u = VEC3_ZERO;
    u.x = v.x;
    u.y = cos(a) * v.y - sin(a) * v.z;
    u.z = sin(a) * v.y + cos(a) * v.z;
    return u;
}
inline Vec3 _rotate_y(Vec3 v, float a) {
    Vec3 u = VEC3_ZERO;
    u.x = cos(a) * v.x + sin(a) * v.z;
    u.y = v.y;
    u.z = -sin(a) * v.x + cos(a) * v.z;
    return u;
}
inline Vec3 _rotate_z(Vec3 v, float a) {
    Vec3 u = VEC3_ZERO;
    u.x = cos(a) * v.x - sin(a) * v.y;
    u.y = sin(a) * v.x + cos(a) * v.y;
    u.z = v.z;
    return u;
}
inline Vec3 _rotate_on_axis(Vec3 v, Vec3 u, float t) {
    // rotation matrix
    // [ a b c
    //   d e f
    //   g h i ]
    const float icos_t = 1 - cos(t);
    float a = cos(t) + u.x * u.x * icos_t;
    float b = u.x * u.y * icos_t - u.z * sin(t);
    float c = u.x * u.z * icos_t + u.y * sin(t);
    float d = u.x * u.y * icos_t + u.z * sin(t);
    float e = cos(t) + u.y * u.y * icos_t;
    float f = u.y * u.z * icos_t - u.x * sin(t);
    float g = u.x * u.z * icos_t - u.y * sin(t);
    float h = u.y * u.z * icos_t + u.x * sin(t);
    float i = cos(t) + u.z * u.z * icos_t;

    Vec3 k = VEC3_ZERO;
    k.x = a * v.x + b * v.y + c * v.z;
    k.y = d * v.x + e * v.y + f * v.z;
    k.z = g * v.x + h * v.y + i * v.z;
    return k;
}

inline Vec3 _scale(Vec3 u, Vec3 v) {
    return u * v;
}
