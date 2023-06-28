#pragma once
#include "vec3.h"
#include "objects.h"
#include "transformation.h"
#include "ray.h"
#include "constant.h"

class Camera {
private:
    std::vector<std::vector<Vec3>> focal_plane_in_world_BASE;
    std::vector<std::vector<Vec3>> focal_plane_in_world;
public:
    // cannot change rotation on the fly
    Vec3 rotation = VEC3_ZERO;
    Vec3 position = VEC3_ZERO;

    float tilted_angle = 0;
    float max_tilt = 80;

    int WIDTH;
    int HEIGHT;
    
    float FOV = 105.0f; // degree
    float focal_length = 10.0f;

    int max_ray_bounce_count = 10;
    int ray_per_pixel = 1;
    float blur_rate = 0.1f;

    float max_range = 50.0f;


    Camera() {
        focal_plane_in_world = v_screen;
        focal_plane_in_world_BASE = focal_plane_in_world;
    }

    // create a ray for (x, y) pixel in screen
    Ray ray(int x, int y) {
        // make the frame blur for anti aliasing by offsetting ray origin
        Vec3 rd = VEC3_ZERO;
        if(blur_rate != 0.0f) {
            rd = random_direction().normalize() * blur_rate;
            rd *= random_val();
        }

        Vec3 endpoint = position + focal_plane_in_world[x][y];

        Vec3 direction = (endpoint - (position + rd)).normalize();
        Ray new_ray;
        new_ray.direction = direction;
        new_ray.origin = position + rd;
        new_ray.max_range = max_range;

        return new_ray;
    }
    // store ray collision on viewport plane for camera rotation
    // this should be called once before start the main loop
    void rays_init() {
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                // store ray collision on viewport for later use
                // this is the position if camera looking direction is (0, 0, 1)
                float viewport_width = 2 * focal_length * tan(deg2rad(FOV/2));
                float viewport_height = viewport_width * HEIGHT/(float)WIDTH;

                float _w = viewport_width * (0.5f - (x - 0.5f)/WIDTH);
                float _h = viewport_height * (0.5f - (y - 0.5f)/HEIGHT);

                focal_plane_in_world[x][y] = Vec3(-_w, _h, focal_length);
                focal_plane_in_world_BASE[x][y] = focal_plane_in_world[x][y];
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
        tilted_angle = 0;
        focal_plane_in_world = focal_plane_in_world_BASE;
    }

    // tilt should not change the camera rotation variable
    void tilt(float a) {
        tilted_angle += a;
        if(fabs(tilted_angle) > deg2rad(max_tilt)) {
            float old_a = tilted_angle;
            if(tilted_angle < 0) tilted_angle = -deg2rad(max_tilt);
            else tilted_angle = deg2rad(max_tilt);
            a -= old_a - tilted_angle;
        }
        // default looking dir
        Vec3 dir = {0, 0, 1};
        dir = -_rotate_y(dir, rotation.y + M_PI/2);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                focal_plane_in_world[x][y] = _rotate_on_axis(focal_plane_in_world[x][y], dir, a);
    }
    void pan(float a) {
        rotate_y(a);
    }
    void move_foward(float ammount) {
        Vec3 dir = get_looking_direction();
        dir *= ammount;
        position += dir;
    }
    void move_left(float ammount) {
        Vec3 dir = get_looking_direction();
        std::swap(dir.x, dir.z); dir.y = 0; dir.x *= -1;
        dir *= ammount;
        position += dir;
    }
    Vec3 get_looking_direction() {
        return Vec3(sin(rotation.y), sin(tilted_angle), cos(rotation.y));
    }
};
