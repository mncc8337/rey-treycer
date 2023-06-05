#pragma once
#include "vec3.h"
#include "objects.h"
#include "helper.h"
#include "rotation.h"
#include "ray.h"

class Camera {
private:
    std::vector<std::vector<Vec3>> focal_plane_in_world_BASE;
    std::vector<std::vector<Vec3>> focal_plane_in_world;

    float tilted_angle = 0;
public:
    Vec3 position = VEC3_ZERO;
    Vec3 rotation = VEC3_ZERO;
    
    float FOV = 105.0f; // degree
    float focal_length = 0.67f;

    int max_ray_bounce_count = 10;
    int ray_per_pixel = 1;
    float blur_rate = 0.000f;


    Camera() {
        focal_plane_in_world = std::vector<std::vector<Vec3>>(WIDTH, v_height);
        focal_plane_in_world_BASE = focal_plane_in_world;
    }

    // create a ray for (x, y) pixel in screen
    Ray ray(int x, int y) {
        // make the frame blur for anti aliasing by offsetting ray origin
        Vec3 rd = VEC3_ZERO;
        if(blur_rate != 0.0f) {
            rd = random_direction().normalize() * blur_rate;
            rd *= random_val();;
        }

        Vec3 endpoint = position + focal_plane_in_world[x][y];

        Vec3 direction = (endpoint - (position + rd)).normalize();
        Ray new_ray;
        new_ray.direction = direction;
        new_ray.origin = position + rd;

        return new_ray;
    }
    // store ray collision on viewport plane for camera rotation
    // this should be called once before start the main loop
    void rays_init() {
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                // store ray collision on viewport for later use
                // this is the position if camera looking direction is (0, 0, 1)
                if(focal_plane_in_world[x][y] == VEC3_ZERO) {
                    float viewport_width = 2 * focal_length * tan(deg2rad(FOV/2));
                    float viewport_height = viewport_width * HEIGHT/(float)WIDTH;

                    float _w = viewport_width * (0.5f - (x - 0.5f)/WIDTH);
                    float _h = viewport_height * (0.5f - (y - 0.5f)/HEIGHT);

                    focal_plane_in_world[x][y] = Vec3(-_w, _h, focal_length);
                    focal_plane_in_world_BASE[x][y] = focal_plane_in_world[x][y];
                }
            }
        // set camera rotation if has
        if(rotation != VEC3_ZERO) {
            rotate(rotation);
            // because rotate_camera() double the variable camera_rotation by 2 so
            rotation /= 2;
        }
    }
    void set_rotation(float x, float y, float z) {
        reset_rotation();
        rotate(x, y, z);
    }
    // rotate the viewport plane around camera position
    void rotate(float x, float y, float z) {
        rotation += Vec3(x, y, z);
        rotate_x(x);
        rotate_y(y);
        rotate_z(z);
    }
    void rotate(Vec3 a) {
        rotation += a;
        rotate_x(a.x);
        rotate_y(a.y);
        rotate_z(a.z);
    }
    void rotate_x(float a) {
        rotation += Vec3(a, 0, 0);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                focal_plane_in_world[x][y] = _rotate_x(focal_plane_in_world[x][y], a);
    }
    void rotate_y(float a) {
        rotation += Vec3(0, a, 0);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                focal_plane_in_world[x][y] = _rotate_y(focal_plane_in_world[x][y], a);
    }
    void rotate_z(float a) {
        rotation += Vec3(0, 0, a);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                focal_plane_in_world[x][y] = _rotate_z(focal_plane_in_world[x][y], a);
    }
    void reset_rotation() {
        rotation = VEC3_ZERO;
        focal_plane_in_world = focal_plane_in_world_BASE;
    }
    void tilt(float a) {
        tilted_angle += a;
        // default looking dir
        Vec3 dir = {0, 0, 1};
        dir = -_rotate_y(dir, rotation.y + pi/2);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                focal_plane_in_world[x][y] = _rotate_on_axis(focal_plane_in_world[x][y], dir, a);
    }
    void pan(float a) {
        rotate_y(a);
    }
    void move_foward(float ammount) {
        Vec3 dir = Vec3(sin(rotation.y), sin(tilted_angle), cos(rotation.y));
        dir *= ammount;
        position += dir;
    }
    void move_left(float ammount) {
        Vec3 dir = Vec3(-cos(rotation.y), 0, sin(rotation.y));
        dir *= ammount;
        position += dir;
    }
};
