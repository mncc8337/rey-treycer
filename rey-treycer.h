#ifndef REYTREYCER_H
#define REYTREYCER_H

#include <thread>
#include <stack>

#include "camera.h"
#include "constant.h"
#include "objects.h"
#include "rng.h"

class ReyTreycer {
private:
    // TODO: better naming
    
    // how many columns a single thread will draw
    int column_threads;
    // how many rows a single thread will draw
    int row_threads;
    // the width of the rectangle a thread will draw
    int thread_width;
    // the height of the rectangle a thread will draw
    int thread_height;
    // a vector to store all draw threads
    std::vector<std::thread> threads;

    // get background light
    Vec3 get_environment_light(Vec3 dir) {
        float level = (dir.y + 1) / 2;
        return lerp(down_sky_color, up_sky_color, level);
    }

    // get closest hit of a ray
    HitInfo ray_collision(Ray* ray) {
        HitInfo closest_hit;
        closest_hit.distance = FLOAT_MAX;

        // find the first intersect point in all objects
        for(Object* obj: objects) {
            if(!obj->visible) continue;

            // only calculate uv if it is not ColorTexture
            bool calculate_uv = obj->get_material().texture->get_type() != TEX_COLOR;

            HitInfo h;
            if(obj->is_sphere())
                h = ray->cast_to_sphere(obj, calculate_uv);
            else
                h = ray->cast_to_mesh(obj, calculate_uv);

            // get the closest hit
            if(h.did_hit and h.distance < closest_hit.distance) {
                closest_hit = h;
                closest_hit.object = obj;
            }
        }

        return closest_hit;
    }

    // get ray traced color from pixel (x, y)
    Vec3 ray_trace(int x, int y) {
        Vec3 ray_color = WHITE;
        Vec3 incomming_light = BLACK;

        Ray ray = camera.ray(x, y);

        std::stack<float> ior_stack;
        ior_stack.push(environment_ior);

        float surrounding_volume_density = 0.0;
        Vec3 surrounding_volume_radiance(0.0, 0.0, 0.0);

        int bounces = 0;
        while(bounces < camera.max_ray_bounce_count) {
            HitInfo h = ray_collision(&ray);

            if(!h.did_hit) {
                incomming_light += ray_color * get_environment_light(ray.direction);
                break;
            }

            SurfaceInfo inf; inf.u = h.u; inf.v = h.v; inf.normal = h.normal;
            Vec3 color = h.material.texture->get_texture(inf);

            if(surrounding_volume_density > 0.0) {
                float scattering_distance = -log(random_val()) / surrounding_volume_density;

                if(scattering_distance < h.distance) {
                    // hit the particle
                    float transmittance = exp(-surrounding_volume_density * scattering_distance);
                    Vec3 radiance = surrounding_volume_radiance * (1.0 - transmittance);
                    incomming_light += ray_color * radiance;
                    ray_color *= transmittance;
                    ray.origin += ray.direction * scattering_distance;
                    ray.direction = random_direction();
                    bounces++;
                    continue;
                }
            }

            if(h.material.volume_density < 1.0) {
                if(h.front_face) {
                    surrounding_volume_density += h.material.volume_density;
                    surrounding_volume_radiance += h.material.emission_strength * color;
                } else {
                    surrounding_volume_density -= h.material.volume_density;
                    surrounding_volume_radiance -= h.material.emission_strength * color;
                    if(surrounding_volume_density < 0.0) {
                        surrounding_volume_density = 0.0;
                        surrounding_volume_radiance = VEC3_ZERO;
                    }
                }
                ray.origin = h.point + ray.direction * EPSILON;
                // recalculate again to account for smoke
                continue;
            }

            if(h.material.ior > 0.0) {
                float current_ior = ior_stack.top();
                float ior_ratio = current_ior / h.material.ior;
                if(!h.front_face) {
                    if(ior_stack.size() > 1) {
                        ior_stack.pop();
                    }
                    ior_ratio = current_ior / ior_stack.top();
                }

                float cos_theta = abs(ray.direction.dot(h.normal));
                bool cannot_refract = ior_ratio * ior_ratio * (1.0 - cos_theta * cos_theta) > 1.0;

                if((cannot_refract or reflectance(cos_theta, ior_ratio) > random_val()) and !_equal(ior_ratio, 1.0f)) {
                    ray.direction = reflection(h.normal, ray.direction);
                    if(!h.front_face and ior_stack.size() >= 1) {
                        ior_stack.push(current_ior);
                    }
                } else {
                    ray.direction = refraction(h.normal, ray.direction, ior_ratio);
                    if(h.front_face) {
                        ior_stack.push(h.material.ior);
                    }
                }
            }
            else {
                Vec3 diffuse_direction = (h.normal + random_direction()).normalize();
                Vec3 specular_direction = reflection(h.normal, ray.direction);
                ray.direction = lerp(specular_direction, diffuse_direction, h.material.roughness);
            }

            ray.origin = h.point + ray.direction * EPSILON;

            ray_color = ray_color * color;
            incomming_light += ray_color * h.material.emission_strength;

            bounces++;
        }

        return incomming_light;
    }

    // start a thread to ray trace pixels in range (from_x, from_y) to (to_x, to_y)
    void drawing_in_rectangle(int from_x, int to_x, int from_y, int to_y) {
        for(int x = from_x; x <= to_x; x++)
            for(int y = from_y; y <= to_y; y++) {
                Vec3 draw_color = BLACK;

                int lazy_mode_condition = x + y * WIDTH + (WIDTH % 2 == 0 and y % 2 == 1);
                if(!lazy_mode or lazy_mode_condition % 2 != rendered_count % 2) {
                    // make more ray per pixel for more accurate color in one frame
                    // but decrease performance
                    for(int k = 1; k <= camera.ray_per_pixel; k++) {
                        draw_color += ray_trace(x, y);
                    }
                    draw_color /= camera.ray_per_pixel;

                    // check if color is NaN or not (idk why this happended lol), fix dark acne
                    if(draw_color.x != draw_color.x or draw_color.y != draw_color.y or draw_color.z != draw_color.z)
                        continue;

                    // progressive rendering
                    float w = 1.0f / (rendered_count + 1.0f);
                    // later frames have less impact than previous frames
                    if(screen_color[x][y] != BLACK)
                        draw_color = screen_color[x][y] * (1 - w) + draw_color * w;
                    screen_color[x][y] = draw_color;
                }
            }
    }

public:
    int WIDTH;
    int HEIGHT;

    // current rendered frames
    int rendered_count = 0;

    std::vector<std::vector<Vec3>> screen_color;

    // environment variable
    float environment_ior = RI_AIR;
    Vec3 up_sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
    Vec3 down_sky_color = WHITE;

    // ray trace pixel like a checker board per frame
    // reduce render time by half
    // only turn on for debug/design
    bool lazy_mode = false;

    // all object pointers in the scene
    std::vector<Object*> objects;

    // the camera
    Camera camera;

    // NOTE: thread_count should be even
    ReyTreycer(int width = 1280, int height = 720, int thread_count = 4) {
        WIDTH = width;
        HEIGHT = height;
        camera.WIDTH = width;
        camera.HEIGHT = height;

        std::vector<int> dividers = get_dividers(thread_count);
        column_threads = dividers[int(dividers.size()/2)];
        row_threads = thread_count / column_threads;
        thread_width = width / column_threads;
        thread_height = height / row_threads;

        screen_color = std::vector<std::vector<Vec3>>(MAX_WIDTH, v_height);
    }
    // get the object on pixel (x, y)
    HitInfo get_collision_on(int x, int y) {
        Ray ray = camera.ray(x, y);
        return ray_collision(&ray);
    }
    // get the number of running draw thread
    int get_running_thread_count() {
        return threads.size();
    }
    // update screen geometry
    void update_size(int width, int height) {
        WIDTH = width;
        HEIGHT = height;
        camera.WIDTH = width;
        camera.HEIGHT = height;
        thread_width = width / column_threads;
        thread_height = height / row_threads;
    }

    // draw and calculate delay
    void draw_frame() {
        // start all draw thread
        for(int w = 0; w < column_threads; w++)
            for(int h = 0; h < row_threads; h++) {
                int draw_from_x = w * thread_width;
                int draw_to_x = (w + 1) * thread_width - 1;
                int draw_from_y = h * thread_height;
                int draw_to_y = (h + 1) * thread_height - 1;
                threads.push_back(std::thread(&ReyTreycer::drawing_in_rectangle, this, draw_from_x, draw_to_x, draw_from_y, draw_to_y));
            }
        // wait till all threads are finished
        for(int i = 0; i < (int)threads.size(); i++)
            threads[i].join();
        // clear the vector for later use
        threads.clear();

        rendered_count++;
    }

    void add_object(Object* obj) {
        objects.push_back(obj);
    }
    void remove_object(Object* obj) {
        for(int i = 0; i < (int)objects.size(); i++)
            if(objects[i] == obj) {
                objects.erase(objects.begin() + i);
                return;
            }
    }
};

// predefined procedural textures

// use normal map as texture
inline Vec3 normal_map(SurfaceInfo h) {
    return (h.normal + Vec3(1, 1, 1)) / 2;
}
// checker texture
inline Vec3 checker(SurfaceInfo h) {
    h.normal = _rotate(h.normal, -h.object_rotation);
    int square_size = 10;
    return Vec3(0, 1, 0) * (sin(square_size * h.normal.x) * sin(square_size * h.normal.y) * sin(square_size * h.normal.z) > 0);
}

#endif
