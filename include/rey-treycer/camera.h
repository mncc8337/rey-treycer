#ifndef CAMERA_H
#define CAMERA_H

#include "objects.h"
#include "ray.h"
#include "rng.h"

class Camera {
private:
    // a vector to store pixels position in world to calculate ray directions
    std::vector<std::vector<Vec3>> pixel_in_world;
public:
    Vec3 position = VEC3_ZERO;

    float panned_angle = 0;
    float tilted_angle = 0;
    // avoid weird camera rotation, in degree
    float max_tilt = 80;

    // the width of viewport
    int WIDTH;
    // the height of viewport
    int HEIGHT;
    
    // field of view in degree, range [1, 179]
    float FOV = 75.0f;

    float focus_distance = 10.0f;
    // how much light can enter the camera, more light -> more blur
    float aperture = 0.02;
    // for anti-aliasing
    float diverge_strength = 0.001;

    // how many times a light ray can bounce
    int max_ray_bounce_count = 10;
    // how many times to calculate color for a pixel
    int ray_per_pixel = 1;

    // max distance the camera can see
    float max_range = 50.0f;


    Camera() {
        pixel_in_world = std::vector<std::vector<Vec3>>(MAX_WIDTH, v_height);
    }

    // create a ray for pixel (x, y)
    Ray ray(int x, int y) {
        // offset ray origin for defocus effect
        Vec3 defocus_jitter = random_point_in_circle() * aperture / 2;
        // offset viewpoint for anti-aliasing
        Vec3 jitter = random_point_in_circle() * diverge_strength;

        Vec3 up_dir = get_up_direction();
        Vec3 right_dir = get_right_direction();

        Vec3 origin = position + right_dir * defocus_jitter.x + up_dir * defocus_jitter.y;
        Vec3 viewpoint = (position + pixel_in_world[x][y] * focus_distance) + right_dir * jitter.x + up_dir * jitter.y;

        Vec3 direction = (viewpoint - origin).normalize();
        Ray new_ray;
        new_ray.direction = direction;
        new_ray.origin = origin;
        new_ray.max_range = max_range;

        return new_ray;
    }
    // calculate position of pixels
    // this is the position if camera looking direction is (0, 0, -1)
    void init() {
        panned_angle = 0.0f;
        tilted_angle = 0.0f;

        float viewport_width = 1;
        float viewport_height = (float)HEIGHT/(float)WIDTH;
        float f = 0.5f / tan(deg2rad(FOV/2));

        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                float _w = viewport_width * (0.5f - (x - 0.5f)/(float)WIDTH);
                float _h = viewport_height * (0.5f - (y - 0.5f)/(float)HEIGHT);

                pixel_in_world[x][y] = Vec3(-_w, _h, -f).normalize();
            }
    }

    void tilt(float a) {
        // clamp tilted_angle to [-max_tilt, max_tilt]
        float old_tilted_angle = tilted_angle;
        tilted_angle += a;
        float rad_max_tilt = deg2rad(max_tilt);
        tilted_angle = fmax(tilted_angle, -rad_max_tilt);
        tilted_angle = fmin(tilted_angle, rad_max_tilt);
        a = tilted_angle - old_tilted_angle;

        Vec3 cam_right = get_right_direction();
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                pixel_in_world[x][y] = _rotate_on_axis(pixel_in_world[x][y], cam_right, a);
    }
    void pan(float a) {
        panned_angle += a;
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++)
                pixel_in_world[x][y] = _rotate_y(pixel_in_world[x][y], a);
    }
    // move the camera foward
    void move_foward(float ammount) {
        Vec3 dir = get_looking_direction();
        dir *= ammount;
        position += dir;
    }
    // move the camera to the right
    void move_right(float ammount) {
        Vec3 dir = get_right_direction();
        dir *= ammount;
        position += dir;
    }
    Vec3 get_looking_direction_XZ() {
        return -Vec3(sin(panned_angle), 0, cos(panned_angle));
    }
    Vec3 get_looking_direction() {
        Vec3 dir_xz = get_looking_direction_XZ();
        return _rotate_on_axis(dir_xz, {-dir_xz.z, 0, dir_xz.x}, tilted_angle);
    }
    Vec3 get_right_direction() {
        Vec3 dir_xz = get_looking_direction_XZ();
        // we dont deal with roll so this is simple enough
        return {-dir_xz.z, 0, dir_xz.x};
    }
    Vec3 get_up_direction() {
        return -get_looking_direction().cross(get_right_direction());
    }
};

#endif
