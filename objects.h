#pragma once
#include <vector>
#include <algorithm>
#include "transformation.h"
#include "vec3.h"
#include "constant.h"
#include "material.h"

class Sphere {
public:
    Vec3 centre = VEC3_ZERO;
    float radius = 1;
    Material material;
};
class Triangle {
public:
    Vec3 vert[3] = {VEC3_ZERO, VEC3_ZERO, VEC3_ZERO};
    Material material;
};
// a bunch of triangle is called mesh
class Mesh {
private:
    Vec3 position = VEC3_ZERO;
    Vec3 rotation = VEC3_ZERO;
    Vec3 scale = Vec3(1, 1, 1);
    Material material;

public:
    std::vector<Triangle> tris;
    Vec3 AABB_min = VEC3_ZERO;
    Vec3 AABB_max = VEC3_ZERO;

    void calculate_AABB() {
        Vec3 min =  Vec3(999999,999999,999999);
        Vec3 max = -Vec3(999999,999999,999999);
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
    void set_position(Vec3 pos) {
        for(int i = 0; i < (int)tris.size(); i++) {
            for(int j = 0; j < 3; j++)
                tris[i].vert[j] += -position + pos;
        }
        position = pos;
    }
    Vec3 get_position() {
        return position;
    }
    void set_rotation(Vec3 a) {
        for(int i = 0; i < (int)tris.size(); i++) {
            for(int j = 0; j < 3; j++) {
                Vec3* pos = &(tris[i].vert[j]);
                (*pos) = (*pos) - position;
                (*pos) = _rotate(*pos, -rotation + a);
                (*pos) = (*pos) + position;
            }
        }
        rotation = a;
    }
    Vec3 get_rotation() {
        return rotation;
    }
    void set_scale(Vec3 v) {
        for(int i = 0; i < (int)tris.size(); i++)
            for(int j = 0; j < 3; j++) {
                Vec3* pos = &(tris[i].vert[j]);
                (*pos) = (*pos) - position;
                (*pos) = _rotate_x(*pos, -rotation.x);
                (*pos) = _rotate_y(*pos, -rotation.y);
                (*pos) = _rotate_z(*pos, -rotation.z);

                // scale
                Vec3 factor = v/scale;
                (*pos) = _scale(*pos, factor);

                (*pos) = _rotate_x(*pos, rotation.x);
                (*pos) = _rotate_y(*pos, rotation.y);
                (*pos) = _rotate_z(*pos, rotation.z);
                (*pos) = (*pos) + position;
            }
        scale = v;
    }
    Vec3 get_scale() {
        return scale;
    }
    void set_material(Material mat) {
        for(int i = 0; i < (int)tris.size(); i++) {
            tris[i].material = mat;
        }
        material = mat;
    }
    Material get_material() {
        return material;
    }
};

class ObjectContainer {
private:
    std::vector<int> sphere_array_middle_empty_space;
    std::vector<int> mesh_array_middle_empty_space;
public:
    Sphere spheres[100000];
    bool spheres_available[100000];
    int sphere_array_length = 0;
    
    Mesh meshes[100000];
    bool meshes_available[100000];
    int mesh_array_length = 0;

    int add_sphere(Sphere s) {
        int index;
        // add new sphere to the middle of the array
        if(!sphere_array_middle_empty_space.empty()) {
            index = sphere_array_middle_empty_space.back();
            sphere_array_middle_empty_space.pop_back();
        }
        // add sphere to the end of the array
        else {
            index = sphere_array_length;
            sphere_array_length++;
        }
        spheres[index] = s;
        spheres_available[index] = true;

        // return index of new added sphere
        return index;
    }
    void remove_sphere(int index) {
        // if sphere is in the middle of the array
        // add index to sphere_array_middle_empty_space
        if(index < sphere_array_length - 1) {
            sphere_array_middle_empty_space.push_back(index);
            std::sort(sphere_array_middle_empty_space.begin(), sphere_array_middle_empty_space.end(), std::greater<int>());

            if(index == sphere_array_length - 1) sphere_array_length -= 1;
        }
        else if(index >= sphere_array_length) return;

        spheres_available[index] = false;
        spheres[index].centre = VEC3_ZERO;
        spheres[index].radius = 0.0f;
    }
    // load sphere new setting
    void set_sphere(int index, Sphere sphere) {
        spheres[index] = sphere;
    }

    // the exact clone but for meshes
    int add_mesh(Mesh m) {
        int index;
        // add new mesh to the middle of the array
        if(!mesh_array_middle_empty_space.empty()) {
            index = mesh_array_middle_empty_space.back();
            mesh_array_middle_empty_space.pop_back();
        }
        // add mesh to the end of the array
        else {
            index = mesh_array_length;
            mesh_array_length++;
        }
        meshes[index] = m;
        meshes_available[index] = true;

        // return index of new added mesh
        return index;
    }
    void remove_mesh(int index) {
        // if mesh is in the middle of the array
        // add index to mesh_array_middle_empty_space
        if(index < mesh_array_length - 1) {
            mesh_array_middle_empty_space.push_back(index);
            std::sort(mesh_array_middle_empty_space.begin(), mesh_array_middle_empty_space.end(), std::greater<int>());

            if(index == mesh_array_length - 1) mesh_array_length -= 1;
        }
        else if(index >= mesh_array_length) return;

        meshes_available[index] = false;
    }
    // load sphere new setting
    void set_mesh(int index, Mesh mesh) {
        meshes[index] = mesh;
    }
};
