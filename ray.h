#pragma once
#include "vec3.h"
#include "constant.h"
#include "material.h"
#include "objects.h"

struct HitInfo {
    bool did_hit = false;
    Vec3 point = VEC3_ZERO;
    Vec3 second_point = VEC3_ZERO;
    float distance = 1e6;
    Vec3 normal = VEC3_ZERO;
    Material material;
};
struct Ray {
    bool hit_from_inside = false;
    Vec3 direction = VEC3_ZERO;
    Vec3 origin = VEC3_ZERO;
    float max_range = 50.0f;
    HitInfo cast_to(Sphere sphere) {
        HitInfo h;

        Vec3 offset_origin = origin - sphere.centre;
        float a = direction.squared_length();
        float b = offset_origin.dot(direction);
        float c = offset_origin.squared_length() - sphere.radius * sphere.radius;
        float D = b * b - a * c;

        if(D >= 0) {
            const float sqrt_D = sqrt(D);
            float distance;
            // hit from outside
            if(!hit_from_inside) 
                distance = (-b - sqrt_D) / a;
            // hit from inside
            else 
                distance = (-b + sqrt_D) / a;
            if(distance < 0 or distance > max_range) return h;

            h.did_hit = true;
            h.distance = distance;
            h.point = origin + direction * distance;
            h.normal = (h.point - sphere.centre).normalize();
            h.material = sphere.material; 
            if(hit_from_inside) {
                h.normal = -h.normal;
                h.material.refractive_index = RI_AIR;
            }
        }
        return h;
    }
    HitInfo cast_to(Triangle tri);
};
