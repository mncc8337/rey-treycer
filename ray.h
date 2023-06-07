#pragma once
#include "vec3.h"
#include "constant.h"
#include "material.h"
#include "objects.h"
#include "helper.h"

struct HitInfo {
    bool did_hit = false;
    Vec3 point = VEC3_ZERO;
    Vec3 second_point = VEC3_ZERO;
    float distance = 1e6;
    Vec3 normal = VEC3_ZERO;
    Material material;
    int object_id = -1;
    std::string object_type = "sphere";
};
struct Ray {
    bool hit_from_inside = false;
    Vec3 direction = VEC3_ZERO;
    Vec3 origin = VEC3_ZERO;
    float max_range = 50.0f;
    bool did_ray_hit_aabb(double t_min, double t_max, AABB aabb) const {
        Vec3 v_invD = {1 / direction.x, 1 / direction.y, 1 / direction.z};
        Vec3 v_t0 = (aabb.min - origin) * v_invD;
        Vec3 v_t1 = (aabb.max - origin) * v_invD;

        // x
        float invD = v_invD.x;
        float t0 = v_t0.x;
        float t1 = v_t1.x;
        if (invD < 0.0f)
            std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;
        // y
        invD = v_invD.y;
        t0 = v_t0.y;
        t1 = v_t1.y;
        if (invD < 0.0f)
            std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;
        // z
        invD = v_invD.z;
        t0 = v_t0.z;
        t1 = v_t1.z;
        if (invD < 0.0f)
            std::swap(t0, t1);
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;

        return true;
    }
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
        h.object_type = CHAR("sphere");
        return h;
    }
    HitInfo cast_to(Triangle tri);
};
