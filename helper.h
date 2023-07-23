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

inline float random_val(double from = 0, double to = 1) {
    static std::uniform_real_distribution<float> distribution(from, to);
    static std::mt19937 generator;
    return distribution(generator);
}
inline float random_val_normal_distribution(double from = 0, double to = 1) {
    static std::normal_distribution<float> distribution(from, to);
    static std::mt19937 generator;
    return distribution(generator);
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
inline Vec3 random_point_in_circle() {
    float angle = random_val() * 2 * M_PI;
    Vec3 point_on_circle(cos(angle), sin(angle), 0);
    return point_on_circle * sqrt(random_val());
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
    float icos = 1 - cosine;
    return r0 + (1-r0) * icos * icos * icos * icos * icos;
}

inline Mesh load_mesh_from(std::string filename = "") {
    Mesh out;

    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cout << "failed to load file\n";
        return out;
    }

    bool has_texture = false;

    // local cache of verts
    std::vector<Vec3> verts;
    std::vector<Vec3> texs;

    while (!f.eof()) {
        char line[128];
        f.getline(line, 128);

        std::stringstream s;
        s << line;
        char junk;

        if (line[0] == 'v') {
            Vec3 v = VEC3_ZERO;
            if(line[1] == 't') {
                has_texture = true;
                s >> junk >> junk >> v.x >> v.y;
                v.x = 1 - v.x;
                v.y = 1 - v.y;
                texs.push_back(v);
            }
            else {
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            }
        }
        else if(line[0] == 'f') {
            if(has_texture) {
                s >> junk;

                std::string tokens[6];
                int nTokenCount = -1;

                while (!s.eof()) {
                    char c = s.get();
                    if (c == ' ' || c == '/')
                        nTokenCount++;
                    else
                        tokens[nTokenCount].append(1, c);
                }

                tokens[nTokenCount].pop_back();

                Triangle tri;
                tri.vert[0] = verts[std::stoi(tokens[0]) - 1];
                tri.vert[1] = verts[std::stoi(tokens[2]) - 1];
                tri.vert[2] = verts[std::stoi(tokens[4]) - 1];

                tri.vert_texture[0] = texs[std::stoi(tokens[1]) - 1];
                tri.vert_texture[1] = texs[std::stoi(tokens[3]) - 1];
                tri.vert_texture[2] = texs[std::stoi(tokens[5]) - 1];
                out.default_tris.push_back(tri);
                out.tris.push_back(tri);
            }
            else {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];
                Triangle tri;
                tri.vert[0] = verts[f[0] - 1];
                tri.vert[1] = verts[f[1] - 1];
                tri.vert[2] = verts[f[2] - 1];
                out.default_tris.push_back(tri);
                out.tris.push_back(tri);
            }
        }
    }

    out.calculate_AABB();
    out.update_material();
    return out;
}
