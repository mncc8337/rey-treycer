#pragma once
#include "vec3.h"
#include "constant.h"
#include "material.h"

struct Sphere {
    Vec3 centre = VEC3_ZERO;
    float radius = 1;
    Material material;
};
struct Triangle {
    Vec3 vert[3];
    Material material;
};
class ObjectContainer {
private:
    std::vector<int> sphere_array_middle_empty_space;
public:
    Sphere spheres[100000];
    bool spheres_available[100000];
    int sphere_array_length = 0;

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
        if(index < sphere_array_length - 1) sphere_array_middle_empty_space.push_back(index);
        else if(index >= sphere_array_length) return;

        spheres_available[index] = false;
        spheres[index].centre = VEC3_ZERO;
        spheres[index].radius = 0.0f;
    }
    // load sphere new setting
    void set_sphere(int index, Sphere sphere) {
        spheres[index] = sphere;
    }
};
