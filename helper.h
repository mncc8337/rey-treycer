#pragma once
#include <random>
#include <math.h>
#include "vec3.h"
#include "constant.h"

inline char* CHAR(std::string str) {
    char* chr = const_cast<char*>(str.c_str());
    return chr;
}
inline float deg2rad(float a) {
    return a / 180 * pi;
}
inline float rad2deg(float a) {
    return a / pi * 180;
}
inline float max(float a, float b) {
    if(a > b) return a;
    return b;
}
inline float min(float a, float b) {
    if(a < b) return a;
    return b;
}
inline Vec3 normalize_color(Vec3 v) {
    float MAX = max(v.x, max(v.y, v.z));
    if(MAX <= 1.0f) return v;
    return v / MAX;
}

std::mt19937 RNG;
std::normal_distribution<float> normal_dist(0, 1);  // N(mean, stddeviation)
std::uniform_real_distribution<float> dist(0.0, 1.0);

inline void set_RNG_seed(int k) {
    RNG.seed(k);
}
inline float random_val() {
    return dist(RNG);
}
inline float random_val_normal_distribution() {
    return normal_dist(RNG);
}
inline Vec3 random_direction() {
    float x = random_val_normal_distribution();
    float y = random_val_normal_distribution();
    float z = random_val_normal_distribution();
    return Vec3(x, y, z).normalize();
}

inline Vec3 random_direction_in_hemisphere(Vec3 n) {
    Vec3 v = random_direction();
    if(n.dot(v) < 0) v = -v;
    return v;
}
inline Vec3 lerp(const Vec3 u, const Vec3 v, const float t) {
    return u * (1-t) + v * t;
}

inline Vec3 reflection(Vec3 n, Vec3 v) {
    return v - 2 * v.dot(n) * n;
}

inline Vec3 refraction(Vec3& n, Vec3& uv, float etai_over_etat) {
    auto cos_theta = fmin(-uv.dot(n), 1.0);
    Vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    Vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.squared_length())) * n;
    return r_out_perp + r_out_parallel;
}
inline float reflectance(float cosine, float ri) {
    // Use Schlick's approximation for reflectance.
    float r0 = (1-ri) / (1+ri);
    r0 *= r0;
    return r0 + (1-r0) * pow((1 - cosine),5);
}
