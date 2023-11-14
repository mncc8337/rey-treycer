#pragma once
#include <thread>
#include <chrono>
#include "camera.h"
#include "objects.h"
#include "image_util.cpp"

class ReyTreycer {
    private:
    int column_threads;
    int row_threads;
    int thread_width;
    int thread_height;
    std::vector<std::thread> threads;

    Vec3 get_environment_light(Vec3 dir) {
        // dir must be normalized
        float level = (dir.y + 1) / 2;
        return lerp(down_sky_color, up_sky_color, level);
    }

    // get closest hit of a ray
    HitInfo ray_collision(Ray* ray) {
        HitInfo closest_hit;
        closest_hit.distance = INFINITY;

        // find the first intersect point in all sphere
        for(Object* obj: objects) {
            if(!obj->visible) continue;

            // only calculate uv if object has textures
            bool calculate_uv = obj->get_material().texture->has_texture();

            HitInfo h;
            if(obj->is_sphere())
                h = ray->cast_to_sphere(obj, calculate_uv);
            else
                h = ray->cast_to_mesh(obj, calculate_uv);

            if(h.did_hit and h.distance < closest_hit.distance) {
                closest_hit = h;
                closest_hit.object = obj;
            }
        }

        return closest_hit;
    }

    Vec3 ray_trace(int x, int y) {
        Vec3 ray_color = WHITE;
        Vec3 incomming_light = BLACK;

        float current_refractive_index = environment_refractive_index;
        std::vector<float> ri_difference_record;

        Ray ray = camera.ray(x, y);

        for(int i = 0; i <= camera.max_ray_bounce_count; i++) {
            HitInfo h = ray_collision(&ray);

            if(h.did_hit) {
                Vec3 old_direction = ray.direction;
                ray.origin = h.point;
                Vec3 diffuse_direction = (h.normal + random_direction()).normalize();
                Vec3 specular_direction = reflection(h.normal, old_direction);

                float rand = random_val();

                if(h.material.transparent) {
                    Vec3 refraction_direction(0, 0, 0);
                    float ri_ratio = current_refractive_index / h.material.refractive_index;

                    float cos_theta = -ray.direction.dot(h.normal);
                    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

                    bool cannot_refract = ri_ratio * sin_theta > 1.0;
                    if((cannot_refract or reflectance(cos_theta, ri_ratio) > rand) and ri_ratio != 1.0f)
                        refraction_direction = specular_direction;
                    else {
                        refraction_direction = refraction(h.normal, old_direction, ri_ratio);
                        float ri_difference = h.material.refractive_index - current_refractive_index;
                        if(h.front_face) {
                            current_refractive_index += ri_difference;
                            ri_difference_record.push_back(ri_difference);
                        }
                        else {
                            if(ri_difference_record.size() == 0) { // if camera is inside object
                                current_refractive_index = RI_AIR;
                            }
                            else {
                                current_refractive_index -= ri_difference_record.back();
                                ri_difference_record.pop_back();
                            }
                        }
                    }

                    ray.direction = refraction_direction;
                }
                else {
                    ray.direction = lerp(specular_direction, diffuse_direction, h.material.roughness);
                }

                Vec3 color = h.material.color;

                SurfaceInfo inf; inf.u = h.u; inf.v = h.v; inf.normal = h.normal;
                if(h.material.texture->has_texture()) {
                    color = h.material.texture->get_texture(inf);
                }
                ray_color = ray_color * color;

                if(h.material.emit_light) {
                    incomming_light += ray_color * color * h.material.emission_strength;
                }
            }
            else {
                incomming_light += ray_color * get_environment_light(ray.direction);
                break;
            }
        }
        return incomming_light;
    }

    void drawing_in_rectangle(int from_x, int to_x, int from_y, int to_y) {
        for(int x = from_x; x <= to_x; x++)
            for(int y = from_y; y <= to_y; y++) {
                Vec3 draw_color = BLACK;

                int lazy_ray_trace_condition = x + y * WIDTH + (WIDTH % 2 == 0 and y % 2 == 1);
                // lazy ray trace
                if(lazy_ray_trace and lazy_ray_trace_condition % 2 == frame_count % 2) {
                    // do nothing
                }
                else {
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
                    float w = 1.0f / (frame_count + 1);
                    if(screen_color[x][y] != BLACK)
                        draw_color = screen_color[x][y] * (1 - w) + draw_color * w;
                    screen_color[x][y] = draw_color;
                }
            }
    }

    public:
    int WIDTH;
    int HEIGHT;

    int frame_count = 0;
    int render_frame_count = 100;
    // time (ms) from last rendered frame to previous rendered frame
    double delay = 0;
    std::vector<std::vector<Vec3>> screen_color;

    // environment variable
    float environment_refractive_index = RI_AIR;
    Vec3 up_sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
    Vec3 down_sky_color = WHITE;

    // ray trace pixel like a checker board per frame
    bool lazy_ray_trace = false;

    // all object pointer in the scene
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
    HitInfo get_collision_on(int x, int y) {
        Ray ray = camera.ray(x, y);
        return ray_collision(&ray);
    }
    int get_running_thread_count() {
        return threads.size();
    }
    void update_size(int width, int height) {
        WIDTH = width;
        HEIGHT = height;
        camera.WIDTH = width;
        camera.HEIGHT = height;
        thread_width = width / column_threads;
        thread_height = height / row_threads;
    }

    void draw_frame() {
        auto start = std::chrono::system_clock::now();

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
        threads.clear();

        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        delay = elapsed.count() * 1000;
        
        frame_count++;
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