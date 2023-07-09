#include <SDL2/SDL_events.h>
#include <thread>

// debug
#include <iostream>
#include <chrono>

#include "camera.h"
#include "constant.h"
#include "graphics.h"
#include "objects.h"

// #include "nlohmann/json.hpp"
// using json = nlohmann::json;

int WIDTH = 320;
int HEIGHT = 180;

int thread_width = WIDTH / column_threads;
int thread_height = HEIGHT / row_threads;
std::vector<std::thread> threads;

bool running = true;
int stationary_frames_count = 0;
bool camera_moving = false;
double delay = 0;
bool keyhold[12];

int render_frame_count = 3;

int mouse_pos_x;
int mouse_pos_y;

Object* selecting_object;

// ray trace pixel like a checker board
// the other pixel color is the average color of neighbor pixel
// reduce render time by half but also reduce image quality
bool lazy_ray_trace = false;

SDL sdl(CHAR("ray tracer"), WIDTH, HEIGHT);

std::vector<std::vector<Vec3>> screen_color(MAX_WIDTH, v_height);
std::vector<std::vector<Vec3>> buffer = screen_color;

Camera camera;

// all object pointer in the scene
std::vector<Object*> objects;
bool sphere_request, mesh_request;
std::string request_mesh_name;
Mesh FOCAL_PLANE;

Vec3 up_sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
Vec3 down_sky_color = WHITE;
Vec3 get_environment_light(Vec3 dir) {
    // dir must be normalized
    float level = (dir.normalize().y + 1) / 2;
    return lerp(down_sky_color, up_sky_color, level);
}

// get closest hit of a ray
HitInfo ray_collision(Ray ray) {
    HitInfo closest_hit;
    closest_hit.distance = INFINITY;
    // find the first intersect point in all sphere
    for(Object* obj: objects) {
        if(!obj->visible) continue;

        HitInfo h;
        if(obj->is_sphere())
            h = ray.cast_to_sphere(obj->get_position(), obj->get_radius(), obj->get_material(), obj->ray_inside);
        else
            h = ray.cast_to_mesh(obj->AABB_min, obj->AABB_max, obj->tris, obj->ray_inside);

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
    float current_refractive_index = RI_AIR;
    Ray ray = camera.ray(x, y);

    for(int i = 1; i <= camera.max_ray_bounce_count; i++) {
        HitInfo h = ray_collision(ray);

        if(h.did_hit) {
            Vec3 old_direction = ray.direction;
            ray.origin = h.point;
            Vec3 diffuse_direction = (h.normal + random_direction()).normalize();
            Vec3 specular_direction = reflection(h.normal, old_direction);
            float rand = random_val();
            bool is_specular_bounce = h.material.metal > rand;

            if(!h.material.transparent) {
                ray.direction = lerp(diffuse_direction, specular_direction, (1 - h.material.roughness) * is_specular_bounce);
            }
            // use refraction ray instead
            else {
                Vec3 refraction_direction(0, 0, 0);
                float ri_ratio = current_refractive_index / h.material.refractive_index;

                float cos_theta = -ray.direction.dot(h.normal);
                float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

                bool cannot_refract = ri_ratio * sin_theta > 1.0;
                if(cannot_refract or reflectance(cos_theta, ri_ratio) > rand)
                    refraction_direction = specular_direction;
                else {
                    refraction_direction = refraction(h.normal, old_direction, ri_ratio);
                    current_refractive_index = h.material.refractive_index;
                    h.object->ray_inside = !h.object->ray_inside;
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

            ray_color = ray_color * lerp(color, h.material.specular_color, is_specular_bounce);
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
            // do not run if frame count is 0
            if(lazy_ray_trace and lazy_ray_trace_condition % 2 == 0 and stationary_frames_count > 0) {
                // calculate number of neighbor
                bool u, d, l, r;
                u = y > 0;
                d = y < HEIGHT - 1;
                l = x > 0;
                r = x < WIDTH - 1;
                int neighbor_count = u + d + l + r;

                if(u) draw_color += screen_color[x][y-1];
                if(d) draw_color += screen_color[x][y+1];
                if(l) draw_color += screen_color[x-1][y];
                if(r) draw_color += screen_color[x+1][y];
                draw_color /= neighbor_count;
            }
            else {
                // make more ray per pixel for more accurate color in one frame
                // but decrease performance
                for(int k = 1; k <= camera.ray_per_pixel; k++) {
                    draw_color += ray_trace(x, y);
                }
                draw_color /= camera.ray_per_pixel;
            }

            // progressive rendering
            float w = 1.0f / (stationary_frames_count + 1);
            draw_color = screen_color[x][y] * (1 - w) + draw_color * w;
            buffer[x][y] = draw_color;
        }
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
            threads.push_back(std::thread(drawing_in_rectangle, draw_from_x, draw_to_x, draw_from_y, draw_to_y));
        }
    // wait till all threads are finished
    for(int i = 0; i < (int)threads.size(); i++)
        threads[i].join();
    threads.clear();

    // copy buffer to screen
    screen_color = buffer;

    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    delay = elapsed.count() * 1000;
    
    stationary_frames_count++;
}

void update_camera() {
    float tilted_a = camera.tilted_angle;
    float panned_a = camera.panned_angle;
    camera.reset_rotation();
    camera.WIDTH = WIDTH;
    camera.HEIGHT = HEIGHT;
    camera.init();

    camera.tilt(tilted_a);
    camera.pan(panned_a);

    thread_width = WIDTH / column_threads;
    thread_height = HEIGHT / row_threads;

    stationary_frames_count = 0;
}

void draw_to_window() {
    while(running) {
        if(stationary_frames_count <= render_frame_count)
            draw_frame();
    }
}

// a small object mananager
Sphere spheres[(int)1e5];
int spheres_array_length = 0;
std::vector<int> middle_space1;

Mesh meshes[(int)1e5];
int meshes_array_length = 0;
std::vector<int> middle_space2;

void add_sphere() {
    Sphere sphere;
    sphere.set_radius(1);
    sphere.set_position(VEC3_ZERO);

    int index;
    if(middle_space1.empty()) {
        index = spheres_array_length;
        spheres_array_length++;
    }
    else {
        index = middle_space1.front();
        middle_space1.erase(middle_space1.begin());
    }
    spheres[index] = sphere;
    objects.push_back(spheres + index);
    selecting_object = spheres + index;
}
void add_mesh() {
    Mesh mesh = load_mesh_from(request_mesh_name);

    int index;
    if(middle_space2.empty()) {
        index = meshes_array_length;
        meshes_array_length++;
    }
    else {
        index = middle_space2.front();
        middle_space2.erase(middle_space2.begin());
    }
    meshes[index] = mesh;
    objects.push_back(meshes + index);
    selecting_object = meshes + index;
}
void remove_object(Object* obj) {
    selecting_object = nullptr;
    if(obj == &FOCAL_PLANE) return;
    for(int i = 0; i < (int)objects.size(); i++)
        if(objects[i] == obj) {
            objects.erase(objects.begin() + i);
            break;
        }

    // find the removed object and update middle space vector
    if(obj->is_sphere()) {
        for(int i = 0; i < spheres_array_length; i++) {
            if(spheres + i == obj) {
                middle_space1.push_back(i);
                return;
            }
        }
    }
    else {
        for(int i = 0; i < meshes_array_length; i++) {
            if(meshes + i == obj) {
                middle_space2.push_back(i);
                return;
            }
        }
    }
}

float delta_time = 0;
int main() {
    // spawn the focal plane
    Mesh FOCAL_PLANE = load_mesh_from("default_model/plane.obj");
    FOCAL_PLANE.visible = false;
    objects.push_back(&FOCAL_PLANE);

    Sphere sphere;
    sphere.set_radius(5);
    sphere.material.texture.load_image("texture/earth.jpg");
    sphere.material.texture.sphere_texture = true;
    objects.push_back(&sphere);

    // camera setting
    camera.position.z = -10;
    camera.FOV = 105;
    camera.focal_length = 10.0f;
    camera.max_range = 100;
    camera.max_ray_bounce_count = 50;
    camera.ray_per_pixel = 1;

    camera.init();

    std::thread draw_thread(draw_to_window);

    while(running) {
        auto start = std::chrono::system_clock::now();

        // force render
        if(sphere_request or mesh_request)
            stationary_frames_count = 0;
        if(sphere_request)
            add_sphere();
        if(mesh_request)
            add_mesh();
        sphere_request = false;
        mesh_request = false;

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
                    selecting_object = hit.object;
                }
                else selecting_object = nullptr;
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

        // handling keyboard signal
        float speed = 5.0f * delta_time;
        float rot_speed = 0.8f * delta_time;
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

        sdl.gui(
            &screen_color,
            &lazy_ray_trace, &render_frame_count, &stationary_frames_count, delay,
            &WIDTH, &HEIGHT,
            &objects, selecting_object,
            &sphere_request, &mesh_request, &request_mesh_name,
            &remove_object,
            &camera,
            &up_sky_color, &down_sky_color,
            &running
        );

        if(old_FOV != camera.FOV
                or old_focal_length != camera.focal_length
                or old_max_range != camera.max_range
                or old_blur_rate != camera.blur_rate
                or camera.WIDTH != WIDTH
                or camera.HEIGHT != HEIGHT)
            update_camera();

        sdl.render();

        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        delta_time = elapsed.count();
    }

    // wait for all additional thread to finished
    draw_thread.join();

    sdl.destroy();

    return 0;
}
