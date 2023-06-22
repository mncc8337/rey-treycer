#include <SDL2/SDL_events.h>
#include <thread>

// debug
#include <iostream>
#include <chrono>

#include "camera.h"
#include "constant.h"
#include "graphics.h"
#include "objects.h"
#include "transformation.h"

// #include "nlohmann/json.hpp"
// using json = nlohmann::json;

int WIDTH = 256;
int HEIGHT = 150;

int thread_width = WIDTH / column_threads;
int thread_height = HEIGHT / row_threads;

const int thread_count = column_threads * row_threads;
std::vector<std::thread> draw_threads;
bool finish_drawing[thread_count];
int draw_from_x[thread_count];
int draw_to_x[thread_count];
int draw_from_y[thread_count];
int draw_to_y[thread_count];
int finish_thread_count = 0;

bool running = true;
int stationary_frames_count = 0;
bool camera_moving = false;
double delay = 0;
int selecting_object = -1;
std::string selecting_object_type;
bool keyhold[12];

int render_frame_count = 3;

int mouse_pos_x;
int mouse_pos_y;

// ray trace pixel like a checker board
// the other pixel color is the average color of neighbor pixel
// reduce render time by half but also reduce image quality
bool lazy_ray_trace = false;

SDL sdl(CHAR("ray tracer"), WIDTH, HEIGHT);

std::vector<std::vector<Vec3>> screen_color = v_screen;

Camera camera;

// all object in the scene
ObjectContainer object_container;

Vec3 up_sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
Vec3 down_sky_color = WHITE;
Vec3 get_environment_light(Vec3 dir) {
    // dir must be normalized
    float level = (dir.normalize().y + 1) / 2;
    return lerp(down_sky_color, up_sky_color, level);
}

// get closest hit of a ray
HitInfo ray_collision(Ray ray) {
    HitInfo closest_hit_sphere;
    closest_hit_sphere.distance = 1e8; // very far away
    // find the first intersect point in all sphere
    for(int i = 0; i < object_container.sphere_array_length; i++) {
        if(!object_container.spheres_available[i]) continue;
        Sphere sphere = object_container.spheres[i];

        HitInfo h = ray.cast_to(sphere);
        if(h.did_hit and h.distance < closest_hit_sphere.distance) {
            closest_hit_sphere = h;
            closest_hit_sphere.object_type = "sphere";
            closest_hit_sphere.object_id = i;
        }
    }

    HitInfo closest_hit_mesh;
    closest_hit_mesh.distance = 1e6;
    for(int i = 0; i < object_container.mesh_array_length; i++) {
        if(!object_container.meshes_available[i]) continue;
        Mesh mesh = object_container.meshes[i];

        HitInfo h = ray.cast_to(mesh);
        if(h.did_hit and h.distance < closest_hit_mesh.distance) {
            closest_hit_mesh = h;
            closest_hit_mesh.object_type = "mesh";
            closest_hit_mesh.object_id = i;
        }
    }

    HitInfo closest_hit;
    if(closest_hit_mesh.distance < closest_hit_sphere.distance) closest_hit = closest_hit_mesh;
    else closest_hit = closest_hit_sphere;

    return closest_hit;
}
Vec3 ray_trace(int x, int y) {
    Vec3 ray_color = WHITE;
    Vec3 incomming_light = BLACK;
    float current_refractive_index = RI_AIR;
    Ray ray = camera.ray(x, y);

    for(int i = 1; i <= camera.max_ray_bounce_count; i++) {
        HitInfo h = ray_collision(ray);

        if(h.did_hit) {
            Vec3 old_direction = ray.direction;
            ray.origin = h.point;
            Vec3 diffuse_direction = (h.normal + random_direction()).normalize();
            Vec3 specular_direction = reflection(h.normal, old_direction);

            if(!h.material.transparent)
                ray.direction = lerp(specular_direction, diffuse_direction, h.material.roughness).normalize();
            // use refraction ray instead
            else {
                Vec3 refraction_direction(0, 0, 0);
                float ri_ratio = current_refractive_index / h.material.refractive_index;

                float cos_theta = -ray.direction.dot(h.normal);
                float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

                bool cannot_refract = ri_ratio * sin_theta > 1.0;
                if(cannot_refract or reflectance(cos_theta, ri_ratio) > random_val())
                    refraction_direction = specular_direction;
                else {
                    refraction_direction = refraction(h.normal, old_direction, ri_ratio);
                    current_refractive_index = h.material.refractive_index;
                    ray.hit_from_inside = !ray.hit_from_inside;
                }

                ray.direction = refraction_direction;
            }
            
            Vec3 emitted_light = h.material.emission_color * h.material.emission_strength;
            incomming_light += emitted_light * ray_color;
            
            Vec3 color = h.material.color;
            if(h.material.texture.image_texture) {
                if(h.material.texture.sphere_texture)
                    color = h.material.texture.get_sphere_texture(h.normal);
            }

            ray_color = ray_color * color;
        }
        else {
            incomming_light += ray_color * get_environment_light(ray.direction);
            break;
        }
    }
    return incomming_light;
}

void draw_frame() {
    auto start = std::chrono::system_clock::now();
    sdl.enable_drawing();

    // start all draw thread
    finish_thread_count = 0;
    for(int i = 0; i < thread_count; i++)
        finish_drawing[i] = false;
    // wait till all threads are finished
    while(finish_thread_count < thread_count) {
        // if not running then escape
        if(!running) return;
    }

    if(lazy_ray_trace) {
        for(int i = 0; i < (WIDTH >> 1) + (WIDTH % 2 == 1); i++)
            for(int y = 0; y < HEIGHT; y++) {
                int x = i * 2;
                if(y % 2 == 1) x += 1;

                Vec3 c = BLACK;
                // calculate number of neighbor
                bool u, d, l, r;
                u = y > 0;
                d = y < HEIGHT - 1;
                l = x > 0;
                r = x < WIDTH - 1;
                int neighbor_count = u + d + l + r;

                if(u) c += screen_color[x][y-1];
                if(d) c += screen_color[x][y+1];
                if(l) c += screen_color[x-1][y];
                if(r) c += screen_color[x+1][y];
                c /= neighbor_count;

                screen_color[x][y] = c;
                Vec3 normalized = normalize_color(c);
                sdl.draw_pixel(x, y, normalized);
            }
    }

    sdl.disable_drawing();
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    delay = elapsed.count() * 1000;
    
    stationary_frames_count++;
}

void drawing_in_rectangle(int from_x, int to_x, int from_y, int to_y) {
    for(int x = from_x; x <= to_x; x++)
        for(int y = from_y; y <= to_y; y++) {
            int skip_condition = x + y * WIDTH + (WIDTH % 2 == 0 and y % 2 == 1);
            if(lazy_ray_trace and skip_condition % 2 == 0) continue;

            Vec3 draw_color = BLACK;
            // make more ray per pixel for more accurate color in one frame
            // but decrease performance
            for(int k = 1; k <= camera.ray_per_pixel; k++) {
                draw_color += ray_trace(x, y);
            }
            draw_color /= camera.ray_per_pixel;

            // progressive rendering
            // make the color better over time without sacrifice performance
            if(!camera_moving) {
                float w = 1.0f / (stationary_frames_count + 1);
                draw_color = screen_color[x][y] * (1 - w) + draw_color * w;
                screen_color[x][y] = draw_color;
            }
            else screen_color[x][y] = draw_color;

            // draw pixel
            Vec3 normalized = normalize_color(draw_color);
            sdl.draw_pixel(x, y, normalized);
        }
}
void draw_thread_function(int id) {
    while(running) {
        if(finish_drawing[id] == false) {
            drawing_in_rectangle(draw_from_x[id], draw_to_x[id], draw_from_y[id], draw_to_y[id]);
            finish_drawing[id] = true;
            finish_thread_count++;
        }
    }
}
void thread_init() {
    for(int w = 0; w < column_threads; w++)
        for(int h = 0; h < row_threads; h++) {
            int id = h * column_threads + w;
            draw_from_x[id] = w * thread_width;
            draw_to_x[id] = (w + 1) * thread_width - 1;
            draw_from_y[id] = h * thread_height;
            draw_to_y[id] = (h + 1) * thread_height - 1;
        }
}

void input_handler() {
    while(running) {
        while(SDL_PollEvent(&(sdl.event))) {
            SDL_GetMouseState(&mouse_pos_x, &mouse_pos_y);
            sdl.process_gui_event();
            running = sdl.event.type != SDL_QUIT;
            if(!running) break;

            if(sdl.event.type == SDL_MOUSEBUTTONDOWN and !sdl.is_hover_over_gui()) {
                // convert to viewport position
                int w; int h;
                SDL_GetWindowSize(sdl.window, &w, &h);
                mouse_pos_x *= WIDTH / (float)w;
                mouse_pos_y *= HEIGHT / (float)h;

                HitInfo hit = ray_collision(camera.ray(mouse_pos_x, mouse_pos_y));
                if(hit.did_hit) {
                    selecting_object = hit.object_id;
                    selecting_object_type = const_cast<char*>(hit.object_type.c_str());
                }
                else selecting_object = -1;
            }

            bool keydown = sdl.event.type == SDL_KEYDOWN;
            switch(sdl.event.key.keysym.sym) {
                case SDLK_UP:
                    keyhold[0] = keydown;
                    break;
                case SDLK_DOWN:
                    keyhold[1] = keydown;
                    break;
                case SDLK_LEFT:
                    keyhold[2] = keydown;
                    break;
                case SDLK_RIGHT:
                    keyhold[3] = keydown;
                    break;
                case SDLK_w:
                    keyhold[4] = keydown;
                    break;
                case SDLK_s:
                    keyhold[5] = keydown;
                    break;
                case SDLK_a:
                    keyhold[6] = keydown;
                    break;
                case SDLK_d:
                    keyhold[7] = keydown;
                    break;
                case SDLK_x:
                    keyhold[8] = keydown;
                    break;
                case SDLK_z:
                    keyhold[9] = keydown;
                    break;
                case SDLK_LSHIFT:
                    keyhold[10] = keydown;
                    break;
                case SDLK_LCTRL:
                    keyhold[11] = keydown;
                    break;
            }
        }
    }
}

void update_camera() {
    float tilted_a = camera.tilted_angle;
    float panned_a = camera.rotation.y;
    camera.reset_rotation();
    camera.WIDTH = WIDTH;
    camera.HEIGHT = HEIGHT;
    camera.rays_init();

    camera.tilt(tilted_a);
    camera.pan(panned_a);

    thread_width = WIDTH / column_threads;
    thread_height = HEIGHT / row_threads;
    stationary_frames_count = 0;
    thread_init();
}

int main() {
    // test sphere texture
    Sphere earth;
    earth.radius = 5;
    earth.material.texture.load_image("texture/earth.jpg");
    earth.material.texture.sphere_texture = true;
    object_container.add_sphere(earth);

    Sphere sun;
    sun.radius = 30;
    sun.centre.x = -50;
    sun.material.texture.load_image("texture/sun.jpg");
    sun.material.texture.sphere_texture = true;
    object_container.add_sphere(sun);

    Sphere moon;
    moon.radius = 1;
    moon.centre.x = 8;
    moon.centre.y = 5;
    moon.material.texture.load_image("texture/moon.jpg");
    moon.material.texture.sphere_texture = true;
    object_container.add_sphere(moon);

    // camera setting
    camera.position.z = -10;
    camera.FOV = 105;
    camera.focal_length = 0.5f;
    camera.max_range = 100;
    camera.max_ray_bounce_count = 50;
    camera.ray_per_pixel = 1;

    camera.rays_init();

    // start drawing thread at the beginning
    for(int i = 0; i < thread_count; i++) {
        finish_drawing[i] = true;
        draw_threads.push_back(std::thread(draw_thread_function, i));
    }
    thread_init();

    // start input thread
    std::thread input_thread(input_handler);

    while(running) {
        // handling keyboard signal
        float speed = 0.5f;
        float rot_speed = 0.05f;
        if(keyhold[10]) {
            speed *= 2;
        }
        if(keyhold[11]) {
            speed /= 4;
            rot_speed /= 4;
        }
        bool camera_changed = false;
        for(int i = 0; i < 10; i++) camera_changed = camera_changed or keyhold[i];
        if(keyhold[2]) camera.pan(-rot_speed);
        if(keyhold[3]) camera.pan(rot_speed);
        if(keyhold[0]) camera.tilt(rot_speed);
        if(keyhold[1]) camera.tilt(-rot_speed);
        if(keyhold[4]) camera.move_foward(speed);
        if(keyhold[5]) camera.move_foward(-speed);
        if(keyhold[6]) camera.move_left(speed);
        if(keyhold[7]) camera.move_left(-speed);
        if(keyhold[8]) camera.position.y += speed;
        if(keyhold[9]) camera.position.y -= speed;
        camera_moving = camera_changed;

        if(camera_moving) stationary_frames_count = 0;

        float old_FOV = camera.FOV;
        float old_focal_length = camera.focal_length;
        float old_max_range = camera.max_range;
        float old_blur_rate = camera.blur_rate;

        sdl.gui(&lazy_ray_trace, &render_frame_count, &stationary_frames_count, delay,
                &WIDTH, &HEIGHT,
                &object_container, &camera,
                &up_sky_color, &down_sky_color,
                &selecting_object, &selecting_object_type,
                &running);

        if(old_FOV != camera.FOV
                or old_focal_length != camera.focal_length
                or old_max_range != camera.max_range
                or old_blur_rate != camera.blur_rate
                or camera.WIDTH != WIDTH
                or camera.HEIGHT != HEIGHT)
            update_camera();

        if(stationary_frames_count <= render_frame_count)
            draw_frame();

        sdl.render();
    }

    // wait for all additional thread to finished
    input_thread.join();
    for(int i = 0; i < thread_count; i++)
        draw_threads[i].join();

    sdl.destroy();

    return 0;
}
