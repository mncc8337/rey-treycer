#include <SDL2/SDL_events.h>
#include <thread>

// debug
#include <iostream>
#include <chrono>

#include "camera.h"
#include "constant.h"
#include "graphics.h"
#include "objects.h"
#include "texture.h"

// #include "nlohmann/json.hpp"
// using json = nlohmann::json;

int WIDTH = 320;
int HEIGHT = 240;

int thread_width = WIDTH / column_threads;
int thread_height = HEIGHT / row_threads;
std::vector<std::thread> threads;

bool running = true;
int stationary_frames_count = 0;
bool camera_moving = false;
double delay = 0;
bool keyhold[12];
bool camera_control = true;

float environment_refractive_index = RI_AIR;

int render_frame_count = 3;

int mouse_pos_x;
int mouse_pos_y;

Object* selecting_object;

// ray trace pixel like a checker board per frame
bool lazy_ray_trace = false;

SDL sdl(CHAR("ray tracer"), WIDTH, HEIGHT);

std::vector<std::vector<Vec3>> screen_color(MAX_WIDTH, v_height);
std::vector<std::vector<Vec3>> buffer = screen_color;

Camera camera;

// all object pointer in the scene
std::vector<Object*> objects;
Mesh FOCAL_PLANE;

Vec3 up_sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
Vec3 down_sky_color = WHITE;
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

            Info inf; inf.u = h.u; inf.v = h.v; inf.normal = h.normal;
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
            if(lazy_ray_trace and lazy_ray_trace_condition % 2 == stationary_frames_count % 2) {
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
                float w = 1.0f / (stationary_frames_count + 1);
                if(screen_color[x][y] != BLACK)
                    draw_color = screen_color[x][y] * (1 - w) + draw_color * w;
                buffer[x][y] = draw_color;
            }
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
        if(stationary_frames_count < render_frame_count)
            draw_frame();
    }
}

Vec3 normal_map(Info h) {
    return (h.normal + Vec3(1, 1, 1)) / 2;
}

float delta_time = 0;
int main() {
    Material FOCAL_PLANE_MAT;
    FOCAL_PLANE_MAT.texture = new BaseTexture;
    FOCAL_PLANE_MAT.color = Vec3(0.18, 0.5, 1.0);
    FOCAL_PLANE_MAT.transparent = true;
    FOCAL_PLANE_MAT.refractive_index = RI_AIR;

    // spawn the focal plane
    Mesh FOCAL_PLANE = load_mesh_from("default_model/plane.obj");
    FOCAL_PLANE.update_material(); // for some reason this cannot called automatically
    FOCAL_PLANE.visible = false;
    FOCAL_PLANE.set_material(FOCAL_PLANE_MAT);
    objects.push_back(&FOCAL_PLANE);

    // dice
    ImageTexture dice_tex;
    dice_tex.load_image("texture/dice.png");

    Material cube_mat;
    cube_mat.texture = &dice_tex;

    Mesh mesh = load_mesh_from("default_model/cube-uv.obj");
    mesh.update_material();
    mesh.set_material(cube_mat);
    objects.push_back(&mesh);

    // texturing based on normals
    ProceduralTexture normal_map_tex;
    normal_map_tex.set_function(&normal_map);

    Material sphere_mat;
    sphere_mat.texture = &normal_map_tex;

    Sphere sphere;
    sphere.set_material(sphere_mat);
    sphere.set_position({0, 5, 0});
    objects.push_back(&sphere);

    // camera setting
    camera.position.z = 10;
    camera.FOV = 90.0f;
    camera.max_range = 100;
    camera.max_ray_bounce_count = 50;
    camera.ray_per_pixel = 1;

    camera.init();

    std::thread draw_thread(draw_to_window);

    while(running) {
        auto start = std::chrono::system_clock::now();

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

                Ray ray = camera.ray(mouse_pos_x, mouse_pos_y);
                HitInfo hit = ray_collision(&ray);
                if(hit.did_hit) {
                    selecting_object = hit.object;
                }
                else selecting_object = nullptr;
            }

            if(camera_control) {
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

        // handling keyboard signal
        if(camera_control) {
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
            if(keyhold[2]) camera.pan(rot_speed);
            if(keyhold[3]) camera.pan(-rot_speed);
            if(keyhold[0]) camera.tilt(rot_speed);
            if(keyhold[1]) camera.tilt(-rot_speed);
            if(keyhold[4]) camera.move_foward(speed);
            if(keyhold[5]) camera.move_foward(-speed);
            if(keyhold[6]) camera.move_right(-speed);
            if(keyhold[7]) camera.move_right(speed);
            if(keyhold[8]) camera.position.y += speed;
            if(keyhold[9]) camera.position.y -= speed;
            camera_moving = camera_changed;

            if(camera_moving) stationary_frames_count = 0;
        }

        float old_FOV = camera.FOV;
        float old_max_range = camera.max_range;
        float old_focus_distance = camera.focus_distance;

        int objs_state = 0;
        sdl.gui(
            &screen_color,
            &camera_control,
            &lazy_ray_trace, &render_frame_count, &stationary_frames_count, delay, (int)threads.size(),
            &WIDTH, &HEIGHT,
            &objects, selecting_object, &objs_state,
            &camera,
            &up_sky_color, &down_sky_color,
            &running
        );

        switch(objs_state) {
            case 1: // make new
                selecting_object = objects.back();
                stationary_frames_count = 0;
                break;
            case 2: // delete
                selecting_object = nullptr;
                stationary_frames_count = 0;
                break;
        }

        if(old_FOV != camera.FOV
                or old_max_range != camera.max_range
                or old_focus_distance != camera.focus_distance
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
