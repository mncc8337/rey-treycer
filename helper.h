#pragma once
#include <random>
#include <fstream>
#include <iostream>
#include <sstream>
#include "vec3.h"
#include "constant.h"
#include "objects.h"

inline char* CHAR(std::string str) {
    char* chr = const_cast<char*>(str.c_str());
    return chr;
}
inline float deg2rad(float a) {
    return a / 180 * M_PI;
}
inline float rad2deg(float a) {
    return a / M_PI * 180;
}

enum TONEMAPPING {
    RGB_CLAMPING = 0,
};
inline Vec3 tonemap(Vec3 v, int style) {
    switch(style) {
        case RGB_CLAMPING:
            return Vec3(fmin(v.x, 1), fmin(v.y, 1), fmin(v.z, 1));
        default:
            return v;
    }
}
inline Vec3 gamma_correct(Vec3 color, float t) {
    return Vec3(pow(color.x, t), pow(color.y, t), pow(color.z, t));
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

inline Mesh load_mesh_from(std::string sFilename) {
    Mesh out;
    std::ifstream f(sFilename);
    if (!f.is_open()) {
        std::cout << "failed to load file\n";
        return out;
    }

    // Local cache of verts
    std::vector<Vec3> verts;
    while (!f.eof()) {
        char line[128];
        f.getline(line, 128);
        std::stringstream s;
        s << line;
        char junk;
        if (line[0] == 'v') {
            Vec3 v = VEC3_ZERO;
            s >> junk >> v.x >> v.y >> v.z;
            verts.push_back(v);
        }
        if (line[0] == 'f') {
            int f[3];
            s >> junk >> f[0] >> f[1] >> f[2];
            Triangle tri;
            tri.vert[0] = verts[f[0] - 1];
            tri.vert[1] = verts[f[1] - 1];
            tri.vert[2] = verts[f[2] - 1];
            out.default_tris.push_back(tri);
        }
    }
    out.tris = out.default_tris;
    out.calculate_AABB();
    return out;
}
