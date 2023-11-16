#pragma once
#include <vector>
#include "transformation.h"
#include "material.h"

class Triangle {
public:
    Vec3 vert[3] = {VEC3_ZERO, VEC3_ZERO, VEC3_ZERO};
    // contain vert texture location in Vec2, being Vec3 because im lazy to implement Vec2
    Vec3 vert_texture[3] = {VEC3_ZERO, VEC3_ZERO, VEC3_ZERO};
    Material* material;
};
class Object {
protected: // idk the difference of private and protected
    Vec3 localx = Vec3(1, 0, 0);
    Vec3 localy = Vec3(0, 1, 0);
    Vec3 localz = Vec3(0, 0, 1);

    Vec3 position = VEC3_ZERO;
    Vec3 rotation = VEC3_ZERO;
    Material material;
public:
    bool visible = true;
    // mesh variable
    Vec3 AABB_min = VEC3_ZERO;
    Vec3 AABB_max = VEC3_ZERO;
    std::vector<Triangle> tris;
    // the default triangles use to restore rotation (because i dont know matrix maths lol)
    std::vector<Triangle> default_tris;

    virtual void set_position(Vec3 p) {
        return;
    };
    Vec3 get_position() {
        return position;
    }
    virtual void set_rotation(Vec3 a) {
        return;
    };
    Vec3 get_rotation() {
        return rotation;
    }
    virtual void set_material(Material mat) {
        material = mat;
    };
    Material get_material() {
        return material;
    }
    virtual void set_radius(float r) {
        return;
    }
    virtual float get_radius() {
        return 0;
    }
    virtual void set_scale(Vec3 v) {
        return;
    }
    virtual Vec3 get_scale() {
        return {1, 1, 1};
    }
    virtual bool is_sphere() {
        return false;
    }
    virtual void calculate_AABB() {
        return;
    }
};
class Sphere: public Object {
private:
    float radius = 1;
public:
    void set_position(Vec3 p) {
        position = p;
    }
    void set_rotation(Vec3 a) {
        rotation = a;
    }
    void set_radius(float r) {
        radius = r;
    }
    float get_radius() {
        return radius;
    }
    bool is_sphere() {
        return true;
    }
};

class Mesh: public Object {
private:
    Vec3 scale = Vec3(1, 1, 1);
public:
    void update_material() {
        for(int i = 0; i < (int)tris.size(); i++) {
            tris[i].material = &material;
            default_tris[i].material = &material;
        }
    }
    // calculate Axis Aligned Bounding Box to optimize ray-mesh intersection
    void calculate_AABB() {
        Vec3 min =  Vec3(INFINITY, INFINITY, INFINITY);
        Vec3 max = -Vec3(INFINITY, INFINITY, INFINITY);
        for(auto tri: tris)
            for(int i = 0; i < 3; i++) {
                min.x = fmin(min.x, tri.vert[i].x);
                min.y = fmin(min.y, tri.vert[i].y);
                min.z = fmin(min.z, tri.vert[i].z);
                
                max.x = fmax(max.x, tri.vert[i].x);
                max.y = fmax(max.y, tri.vert[i].y);
                max.z = fmax(max.z, tri.vert[i].z);
            }
        AABB_min = min;
        AABB_max = max;
    }
    void set_position(Vec3 p) {
        for(int i = 0; i < (int)tris.size(); i++) {
            for(int j = 0; j < 3; j++)
                tris[i].vert[j] += -position + p;
        }
        position = p;
    }
    void set_rotation(Vec3 a) {
        // reset
        tris = default_tris;

        for(int i = 0; i < (int)tris.size(); i++) {
            for(int j = 0; j < 3; j++) {
                Vec3* pos = &(tris[i].vert[j]);
                *pos = _scale(*pos, scale);
                *pos = _rotate(*pos, a);
                *pos = *pos + position;
            }
        }
        rotation = a;
    }
    void set_scale(Vec3 v) {
        if(scale.x == 0) scale.x = 1;
        if(scale.x == 0) scale.x = 1;
        if(scale.z == 0) scale.z = 1;
        Vec3 d = v / scale;
        for(int i = 0; i < (int)tris.size(); i++)
            for(int j = 0; j < 3; j++) {
                Vec3* pos = &(tris[i].vert[j]);
                *pos -= position;
                *pos = _scale(*pos, d);
                *pos += position;
            }
        scale = v;
    }
    Vec3 get_scale() {
        return scale;
    }
    void set_material(Material mat) {
        material = mat;
    }
    bool is_sphere() {
        return false;
    }
};