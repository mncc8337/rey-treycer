#ifndef HELPER_H
#define HELPER_H

#include "objects.h"

#include <fstream>
#include <iostream>
#include <sstream>

// various of methods that not fall into any specific category

inline bool _equal_zero(float a) {
    return fabs(a) < EPSILON;
}
inline bool _equal(float a, float b) {
    float diff = fabs(a - b);
    return diff < EPSILON;
}

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
// return a sorted array of dividers of a
inline std::vector<int> get_dividers(int a) {
    std::vector<int> dividers;
    for(int i = 1; i <= a / 2; i++)
        if(a % i == 0) dividers.push_back(i);
    dividers.push_back(a);

    return dividers;
}

// TODO: add more tonemapping methods
enum TONEMAPPING {
    TONEMAP_RGB_CLAMPING = 0,
};
inline Vec3 tonemap(Vec3 v, int style) {
    switch(style) {
        case TONEMAP_RGB_CLAMPING:
            return Vec3(fmin(v.x, 1), fmin(v.y, 1), fmin(v.z, 1));
        default:
            return v;
    }
}
inline Vec3 gamma_correct(Vec3 color, float t) {
    return Vec3(pow(color.x, t), pow(color.y, t), pow(color.z, t));
}

// load a mesh from a *.obj file, simplified
inline Mesh load_mesh_from(std::string filename) {
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
    return out;
}

#endif
