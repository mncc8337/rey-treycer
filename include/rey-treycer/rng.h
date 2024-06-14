#ifndef RNG_H
#define RNG_H

#include <random>
#include "vec3.h"

// generate a uniform random value
inline float random_val(double from = 0, double to = 1) {
    static std::uniform_real_distribution<float> distribution(from, to);
    static std::mt19937 generator;
    return distribution(generator);
}
// generate a normal distributed random value
inline float random_val_normal_distribution(double from = 0, double to = 1) {
    static std::normal_distribution<float> distribution(from, to);
    static std::mt19937 generator;
    return distribution(generator);
}
// generate a random direction
inline Vec3 random_direction() {
    float x = random_val_normal_distribution();
    float y = random_val_normal_distribution();
    float z = random_val_normal_distribution();
    return Vec3(x, y, z).normalize();
}
// generate a random direction in a hemisphere
inline Vec3 random_direction_in_hemisphere(Vec3 n) {
    Vec3 v = random_direction();
    if(n.dot(v) < 0) v = -v;
    return v;
}
// generate a random point in circle, returned Vec3 instead of Vec2 because im lazy to implement Vec2
inline Vec3 random_point_in_circle() {
    float angle = random_val() * 2 * M_PI;
    Vec3 point_on_circle(cos(angle), sin(angle), 0);
    return point_on_circle * sqrt(random_val());
}

#endif
