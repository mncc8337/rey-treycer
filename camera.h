#pragma once
#include "vec3.h"
#include "objects.h"
#include "transformation.h"
#include "ray.h"
#include "constant.h"

class Camera {
private:
    std::vector<std::vector<Vec3>> pixel_in_world_BASE;
    std::vector<std::vector<Vec3>> pixel_in_world;
public:
    Vec3 position = VEC3_ZERO;

    float panned_angle = 0;
    float tilted_angle = 0;
    float max_tilt = 80; // avoid confusion

    int WIDTH;
    int HEIGHT;
    
    float FOV = 105.0f; // degree
    float focal_length = 10.0f;

    int max_ray_bounce_count = 10;
    int ray_per_pixel = 1;
    float blur_rate = 0.1f;

    float max_range = 50.0f;


    Camera() {
        pixel_in_world = std::vector<std::vector<Vec3>>(MAX_WIDTH, v_height);
        pixel_in_world_BASE = pixel_in_world;
    }

    // create a ray for (x, y) pixel in screen
    Ray ray(int x, int y) {
        // defocus effect by offsetting ray origin
        Vec3 rd = VEC3_ZERO;
        if(blur_rate != 0.0f)
            rd = random_direction() * blur_rate;

        Vec3 startpoint = position + rd;
        Vec3 endpoint = position + pixel_in_world[x][y];

        Vec3 direction = (endpoint - startpoint).normalize();
        Ray new_ray;
        new_ray.direction = direction;
        new_ray.origin = startpoint;
        new_ray.max_range = max_range;

        return new_ray;
    }
    // store viewport pixels position for camera rotation
    // this is the position if camera looking direction is (0, 0, 1)
    void init() {
        float viewport_width = 2 * focal_length * tan(deg2rad(FOV/2));
        float viewport_height = viewport_width * HEIGHT/(float)WIDTH;

        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                float _w = viewport_width * (0.5f - (x - 0.5f)/WIDTH);
                float _h = viewport_height * (0.5f - (y - 0.5f)/HEIGHT);

                pixel_in_world[x][y] = Vec3(-_w, _h, focal_length);
                pixel_in_world_BASE[x][y] = pixel_in_world[x][y];
            }
    }
    void reset_rotation() {
        panned_angle = 0;
        tilted_angle = 0;
        pixel_in_world = pixel_in_world_BASE;
    }

    void tilt(float a) {
        // clamp tilted_angle to [-max_tilt, max_tilt]
        float old_tilted_angle = tilted_angle;
        tilted_angle += a;
        float rad_max_tilt = deg2rad(max_tilt);
        tilted_angle = fmax(tilted_angle, -rad_max_tilt);
        tilted_angle = fmin(tilted_angle, rad_max_tilt);
        a = tilted_angle - old_tilted_angle;

        // perpendicular vector of looking dir for rotation
        Vec3 dir = get_looking_direction().cross({0, 1, 0});
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                pixel_in_world[x][y] = _rotate_on_axis(pixel_in_world[x][y], dir, a);
    }
    void pan(float a) {
        panned_angle += a;
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                pixel_in_world[x][y] = _rotate_y(pixel_in_world[x][y], a);
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
        return Vec3(sin(panned_angle), sin(tilted_angle), cos(panned_angle));
    }
};
