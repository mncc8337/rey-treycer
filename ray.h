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
    int object_type = TYPE_SPHERE;
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
        h.object_type = TYPE_SPHERE;
        return h;
    }
    HitInfo cast_to(Triangle tri) {
        Vec3 edgeAB = tri.vert[1] - tri.vert[0];
        Vec3 edgeAC = tri.vert[2] - tri.vert[0];

        //  reverse the triangle so that ray can hit from inside
        if(hit_from_inside) std::swap(edgeAB, edgeAC);

        Vec3 normalVector = edgeAB.cross(edgeAC);
        Vec3 ao = origin - tri.vert[0];
        Vec3 dao = ao.cross(direction);

        float determinant = -(direction.dot(normalVector));
        float invDet = 1 / determinant;

        float dst = ao.dot(normalVector) * invDet;
        float u = edgeAC.dot(dao) * invDet;
        float v = -(edgeAB.dot(dao)) * invDet;
        float w = 1 - u - v;

        HitInfo h;
        h.did_hit = determinant >= 1e-6 && dst >= 0 && u >= 0 && v >= 0 && w >= 0;
        h.point = origin + direction * dst;
        h.normal = normalVector.normalize();
        h.distance = dst;
        h.material = tri.material;
        return h;
    }
    bool cast_to_AABB(Vec3 box_min, Vec3 box_max) {
        Vec3 invDir = 1 / direction;
        Vec3 tMin = (box_min - origin) * invDir;
        Vec3 tMax = (box_max - origin) * invDir;
        Vec3 t1 = Vec3(fmin(tMin.x, tMax.x), fmin(tMin.y,tMax.y), fmin(tMin.z, tMax.z));
        Vec3 t2 = Vec3(fmax(tMin.x, tMax.x), fmax(tMin.y,tMax.y), fmax(tMin.z, tMax.z));
        float tNear = fmax(fmax(t1.x, t1.y), t1.z);
        float tFar = fmin(fmin(t2.x, t2.y), t2.z);
        return tNear <= tFar;
    }
    HitInfo cast_to(Mesh mesh) {
        HitInfo closest;
        closest.did_hit = false;
        closest.distance = 1e6;
        // assume that mesh.calculate_AABB() is called at least once
        // if not collide with AABB then skip
        if(!cast_to_AABB(mesh.AABB_min,mesh.AABB_max)) return closest;
        // find closest hit
        for(auto tri: mesh.tris) {
            HitInfo h = cast_to(tri);
            if(h.did_hit and h.distance < closest.distance)
                closest = h;
        }
        closest.object_type = TYPE_MESH;
        return closest;
    }
};
