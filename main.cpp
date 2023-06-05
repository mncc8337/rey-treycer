#include <thread>

// debug
/* #include "CImg/CImg.h" */
#include <string>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "camera.h"
#include "constant.h"
#include "graphic.h"
#include "objects.h"
#include "rotation.h"

/* using namespace cimg_library; */

const int thread_width = WIDTH / column_threads;
const int thread_height = HEIGHT/ row_threads;

bool running = true;
int frames_count = 0;
int stationary_frames_count = 0;
bool camera_moving = false;

// ray trace pixel like a checker board
// the other pixel color is the average color of neighbor pixel
// reduce render time by half but also reduce image quality
bool lazy_ray_trace = true;

SDL sdl(const_cast<char*>(std::string("ray tracer").c_str()));

std::vector<std::vector<Vec3>> screen_color = v_screen;
HitInfo first_ray_hitinfo[WIDTH][HEIGHT];
std::vector<std::vector<Vec3>> first_ray_direction = v_screen;

Camera camera;

// all object in the scene
ObjectContainer object_container;

const Vec3 sky_color = Vec3(0.51f, 0.7f, 1.0f) * 1.0f;
Vec3 get_environment_light(Vec3 dir) {
    // dir must be normalized
    float level = (dir.normalize().y + 1) / 2;
    return lerp(WHITE, sky_color, level);
}

// get closest hit of a ray
HitInfo ray_collision(Ray ray) {
    HitInfo closest_hit;
    closest_hit.distance = 1e6;
    // find the first intersect point in all sphere
    for(int i = 0; i < object_container.sphere_array_length; i++) {
        if(!object_container.spheres_available[i]) continue;
        Sphere sphere = object_container.spheres[i];

        HitInfo h = ray.cast_to(sphere);
        if(h.did_hit and h.distance < closest_hit.distance) {
            closest_hit = h;
        }
    }
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
        for(int i = 0; i < (WIDTH >> 1); i++)
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
}

bool keyhold[8];
void check_input() {
    while(running) {
	    if(!SDL_WaitEvent(&(sdl.event))) continue;
        running = sdl.event.type != SDL_QUIT;

        bool keyup = sdl.event.type == SDL_KEYUP;
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

        }
    }
}
int main() {
    camera.position = {11.2515, 12.5, -5.5125};
    //camera.rotation = {-0.5, -1.14, 0};
    camera.max_ray_bounce_count = 100;
    camera.ray_per_pixel = 1;
    camera.focal_length = 10.0f;

    Sphere sphere;
    Sphere light;

    light.centre = Vec3(-70, 80, 80);
    light.radius = 100.0f;
    light.material.color = BLACK;
    light.material.roughness = 1.0f;
    light.material.emission_color = WHITE;
    light.material.emission_strength = 1.0f;
    object_container.add_sphere(light);

    light.centre = {4.0f, 8, -4.0f};
    light.radius = 2.0f;
    light.material.color = BLACK;
    light.material.roughness = 1.0f;
    light.material.emission_color = WHITE;
    light.material.emission_strength = 1.0f;
    object_container.add_sphere(light);

    sphere.centre = Vec3(6.0f, 12, 6.0f);
    sphere.radius = 5.0f;
    sphere.material.color = Vec3(0.694, 0.7, 0.71);
    sphere.material.roughness = 0.0f;
    object_container.add_sphere(sphere);

    sphere.centre = Vec3(-8, 8.5, -5.0f);
    sphere.radius = 3.5f;
    sphere.material.color = WHITE;
    sphere.material.roughness = 0.0f;
    sphere.material.transparent = true;
    sphere.material.refractive_index = RI_WATER;
    object_container.add_sphere(sphere);
    sphere.material.transparent = false;

    sphere.centre = Vec3(10.0, 20, 0.0f);
    sphere.radius = 5.0f;
    sphere.material.color = WHITE;
    sphere.material.roughness = 0.0f;
    sphere.material.transparent = true;
    sphere.material.refractive_index = RI_WATER;
    object_container.add_sphere(sphere);
    sphere.material.transparent = false;

    sphere.centre = Vec3(0.0f, -22, 7.0f);
    sphere.radius = 30.0f;
    sphere.material.color = Vec3(0.259, 0.529, 0.96);
    sphere.material.roughness = 0.3f;
    object_container.add_sphere(sphere);

    sphere.centre = Vec3(-4.5f, 11, 4.5f);
    sphere.radius = 4.0f;
    sphere.material.color = Vec3(1, 0.4, 0.4);
    sphere.material.roughness = 0.0f;
    object_container.add_sphere(sphere);

    sphere.centre = Vec3(-15.7f, 8.2, 4.5f);
    sphere.radius = 3.0f;
    sphere.material.color = Vec3(1, 0.78, 0);
    sphere.material.roughness = 0.0f;
    object_container.add_sphere(sphere);

    sphere.centre = Vec3(-20.7f, 2, -5);
    sphere.radius = 3.0f;
    sphere.material.color = WHITE;
    sphere.material.roughness = 1.0f;
    object_container.add_sphere(sphere);

    sphere.centre = Vec3(15.0f, 6, 0.0f);
    sphere.radius = 4.0f;
    sphere.material.color = WHITE;
    sphere.material.roughness = 1.0f;
    object_container.add_sphere(sphere);

    camera.rays_init();
    optimize_ray_cast();

    float elapsed_time= 0;
    float avg_delay = 0;

    // separate input thread and drawing thread so that input is not delayed
    std::thread input_thread(check_input);

    while(running) {
        float speed = 0.5f;
        float rot_speed = 0.1f;
        bool camera_changed = false;
        for(int i = 0; i < 8; i++) camera_changed = camera_changed or keyhold[i];
        if(keyhold[2]) camera.pan(-rot_speed);
        if(keyhold[3]) camera.pan(rot_speed);
        if(keyhold[0]) camera.tilt(rot_speed);
        if(keyhold[1]) camera.tilt(-rot_speed);
        if(keyhold[4]) camera.move_foward(speed);
        if(keyhold[5]) camera.move_foward(-speed);
        if(keyhold[6]) camera.move_left(speed);
        if(keyhold[7]) camera.move_left(-speed);
        if(camera_changed and camera.blur_rate == 0.0f) optimize_ray_cast();
        camera_moving = camera_changed;

        if(camera_moving) {
            std::cout << camera.position.x << ' ' << camera.position.y << ' ' << camera.position.z << '\n';
            std::cout << camera.rotation.x << ' ' << camera.rotation.y << ' ' << camera.rotation.z << '\n';
        }

        auto start = std::chrono::system_clock::now();
        draw_frame();
        sdl.swap_buffer();
        auto end = std::chrono::system_clock::now();

        frames_count++;
        stationary_frames_count += 1;
        if(camera_moving) stationary_frames_count = 0;
        if(stationary_frames_count == 3) {
            camera.blur_rate = 0.0f;
        }
        else if(stationary_frames_count == 0)
            camera.blur_rate = 0.0f;

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "frame " << frames_count << " takes " << elapsed.count() * 1000 << "ms ";
        elapsed_time += elapsed.count() * 1000;
        avg_delay = elapsed_time / frames_count;
        std::cout << "avg " << avg_delay << "ms\n";
    }

    // if(record) {
    //     CImg<int> image(WIDTH, HEIGHT, 1, 3);
    //     for(int x = 0; x < WIDTH; x++)
    //         for(int y = 0; y < HEIGHT; y++) {
    //             Vec3 normalized = normalize_color(screen_color[x][y]);
    //             int COLOR[] = {int(normalized.x * 255), int(normalized.y * 255), int(normalized.z * 255)};
    //             image.draw_point(x, y, COLOR, 1.0f);
    //         }
    //     
    //     auto t = std::time(nullptr);
    //     auto tm = *std::localtime(&t);
    //
    //     std::ostringstream oss;
    //     oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
    //     auto str = "res/" + oss.str() + ".bmp";
    //     char *c = const_cast<char*>(str.c_str());
    //
    //     image.save(c);
    // }
    std::cout << "avg " << avg_delay << "ms\n";

    return 0;
}
