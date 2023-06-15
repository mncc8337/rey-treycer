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

int WIDTH = 128;
int HEIGHT = 75;

int thread_width = WIDTH / column_threads;
int thread_height = HEIGHT/ row_threads;

bool running = true;
int stationary_frames_count = 0;
bool camera_moving = false;
double delay = 0;
int selecting_object = -1;
std::string selecting_object_type;
bool keyhold[12];

int render_frame_count = 3;
float blur_rate = 0.01f;

int mouse_pos_x;
int mouse_pos_y;

// ray trace pixel like a checker board
// the other pixel color is the average color of neighbor pixel
// reduce render time by half but also reduce image quality
bool lazy_ray_trace = false;

SDL sdl(CHAR("ray tracer"), WIDTH, HEIGHT);

std::vector<HitInfo> h_height(MAX_HEIGHT);
std::vector<std::vector<HitInfo>> first_ray_hitinfo(MAX_WIDTH, h_height);

std::vector<std::vector<Vec3>> screen_color = v_screen;
std::vector<std::vector<Vec3>> first_ray_direction = v_screen;

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
    closest_hit_sphere.distance = 1e6;
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


// if the camera is stationary and ray is not randomly offsetted (blur rate is 0.0f)
// then HitInfo of the first ray from the camera is always the same
// this function store that HitInfo into an array to help boost the performance (a lot)
// this should be called after the camera is moved or rotated
// automatically called before main loop start when blur rate is 0.0f
void optimize_ray_cast() {
    // if blur is turned on then skip this
    if(camera.blur_rate != 0.0f) return;

    for(int x = 0; x < WIDTH; x++)
        for(int y = 0; y < HEIGHT; y++) {
            Ray ray = camera.ray(x, y);
            HitInfo h = ray_collision(ray);
            first_ray_hitinfo[x][y] = h;
            first_ray_direction[x][y] = ray.direction;
        }
}
Vec3 ray_trace(int x, int y) {
    Vec3 ray_color = WHITE;
    Vec3 incomming_light = BLACK;
    float current_refractive_index = RI_AIR;
    Ray ray;

    if(camera.blur_rate != 0.0f) ray = camera.ray(x, y);

    for(int i = 1; i <= camera.max_ray_bounce_count; i++) {
        HitInfo h;
        if(camera.blur_rate == 0.0f and i == 1) {
            h = first_ray_hitinfo[x][y];
            ray.direction = first_ray_direction[x][y];
        }
        else h = ray_collision(ray);

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
                float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

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
            ray_color = ray_color * h.material.color;
        }
        else {
            incomming_light += ray_color * get_environment_light(ray.direction);
            break;
        }
    }
    return incomming_light;
}
void drawing_rectangle_thread(int from_x, int to_x, int from_y, int to_y) {
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

            // draw pixel to buffer
            Vec3 normalized = normalize_color(draw_color);
            sdl.draw_pixel(x, y, normalized);
        }
}

void draw_frame() {
    auto start = std::chrono::system_clock::now();
    sdl.enable_drawing();
    std::vector<std::thread> threads;
    for(int w = 0; w < column_threads; w++)
        for(int h = 0; h < row_threads; h++) {
            int from_x = w * thread_width;
            int to_x = (w + 1) * thread_width - 1;
            int from_y = h * thread_height;
            int to_y = (h + 1) * thread_height - 1;
            threads.push_back(std::thread(drawing_rectangle_thread, from_x, to_x, from_y, to_y));
        }
    // wait till all threads are finished
    for(auto& t: threads) t.join();

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
    
    stationary_frames_count += 1;
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
    thread_height = HEIGHT/ row_threads;
    stationary_frames_count = 0;

    draw_frame();
}

int main() {
    camera.HEIGHT = HEIGHT;

    camera.position.z = -10;
    camera.FOV = 105;
    camera.focal_length = 0.5f;
    camera.max_range = 100;
    camera.max_ray_bounce_count = 50;
    camera.ray_per_pixel = 1;
    sdl.set_camera_var(&camera);

    camera.rays_init();
    optimize_ray_cast();

    while(running) {
	    while(SDL_PollEvent(&(sdl.event))) {
            SDL_GetMouseState(&mouse_pos_x, &mouse_pos_y);
            sdl.process_gui_event();
            running = sdl.event.type != SDL_QUIT;

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
        float speed = 0.5f;
        float rot_speed = 0.1f;
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
        if(stationary_frames_count == 3)
            camera.blur_rate = blur_rate;
        else if(stationary_frames_count == 0) {
            camera.blur_rate = 0.0f;
            optimize_ray_cast();
        }

        sdl.gui(&lazy_ray_trace, &render_frame_count, &blur_rate, &camera_moving, &stationary_frames_count, delay, &WIDTH, &HEIGHT, &object_container, &camera, update_camera, &up_sky_color, &down_sky_color, &selecting_object, &selecting_object_type, &running);

        if(camera.WIDTH != WIDTH or camera.HEIGHT != HEIGHT)
            update_camera();

        if(stationary_frames_count <= render_frame_count) draw_frame();

        sdl.render();
    }
    sdl.destroy();
    return 0;
}
