#pragma once
#include "vec3.h"
#include "constant.h"
#include "material.h"
#include "objects.h"
#include "helper.h"

struct HitInfo {
    bool did_hit = false;
    Vec3 point = VEC3_ZERO;
    float distance = INFINITY;
    Vec3 normal = VEC3_ZERO;
    Material material;
    Object* object = nullptr;
};
struct Ray {
    Vec3 direction = VEC3_ZERO;
    Vec3 origin = VEC3_ZERO;
    float max_range = 50.0f;
    HitInfo cast_to_sphere(Vec3 centre, float radius, Material mat) {
        HitInfo h;

        Vec3 offset_origin = origin - centre;
        float a = direction.squared_length();
        float b = offset_origin.dot(direction);
        float c = offset_origin.squared_length() - radius * radius;
        // fix dark acne (idk why this work lol)
        if(c < 1e-6) c = 0;
        float discriminant = b * b - a * c;

        float l = c; // this is an approximation, the true equation is `l = offset_origin.length() - radius`
        // determine wether the ray origin is in the sphere or not
        bool inside_object = l <= 0;
        if(l == 0 and b > 0) // if ray origin is lying on sphere surface and ray is going out then there must be no hit
            return h;

        if(discriminant >= 0) {
            const float sqrt_discriminant = sqrt(discriminant);
            float distance;
            // hit front face
            if(!inside_object)
                distance = (-b - sqrt_discriminant) / a;
            // hit back face
            else 
                distance = (-b + sqrt_discriminant) / a;
            if(distance < 1e-6 or distance > max_range) return h;

            h.did_hit = true;
            h.distance = distance;
            h.point = origin + direction * distance;
            h.normal = (h.point - centre) / radius;
            h.material = mat;
            if(inside_object) {
                h.normal = -h.normal;
                h.material.refractive_index = RI_AIR;
            }
        }
        return h;
    }
    HitInfo cast_to_triangle(Triangle tri) {
        HitInfo h;

        Vec3 edgeAB = tri.vert[1] - tri.vert[0];
        Vec3 edgeAC = tri.vert[2] - tri.vert[0];

        Vec3 normalVector = edgeAB.cross(edgeAC);
        float determinant = -direction.dot(normalVector);

        bool hit_backward = false;
        if(determinant < 1e-6) {
            // then ray hit the triangle from the back face
            hit_backward = true;
            normalVector *= -1;
            determinant *= -1;
            std::swap(edgeAB, edgeAC);

            if(determinant < 1e-6) return h;
        }

        float invDet = 1 / determinant;
        Vec3 ao = origin - tri.vert[0];

        float dst = ao.dot(normalVector) * invDet;
        if(dst > max_range or dst <= 1e-6) return h;

        Vec3 dao = ao.cross(direction);

        float u = edgeAC.dot(dao) * invDet;
        if(u < 0) return h;

        float v = -(edgeAB.dot(dao)) * invDet;
        if(v < 0) return h;

        float w = 1 - u - v;
        if(w < 0) return h;

        h.did_hit = true;
        h.point = origin + direction * (dst * 0.99999); // fix ray origin lie on triangle surface and cause dark acne
        h.normal = normalVector.normalize();
        h.distance = dst;
        h.material = tri.material;
        if(hit_backward) {
            h.material.refractive_index = RI_AIR;
        }
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
    HitInfo cast_to_mesh(Vec3 AABB_min, Vec3 AABB_max, std::vector<Triangle> tris) {
        HitInfo closest;
        closest.did_hit = false;
        closest.distance = INFINITY;
        // assume that mesh.calculate_AABB() is called at least once
        // if not collide with AABB then skip
        if(!cast_to_AABB(AABB_min, AABB_max)) return closest;
        // find closest hit
        for(Triangle tri: tris) {
            HitInfo h = cast_to_triangle(tri);
            if(h.did_hit and h.distance < closest.distance)
                closest = h;
        }
        return closest;
    }
};
