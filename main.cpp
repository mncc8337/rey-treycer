#include <SDL2/SDL_events.h>
#include "rey-treycer.h"
#include "graphics.h"

int WIDTH = 1280;
int HEIGHT = 720;

ReyTreycer rt(WIDTH, HEIGHT, 8);
SDL sdl(CHAR("ray tracer"), WIDTH, HEIGHT);
const Uint8 *keys = SDL_GetKeyboardState(NULL);

Camera* camera = &rt.camera;

const float SPEED = 5.0f;
const float ROT_SPEED = 0.8f;

bool running = true;
bool camera_control = true;

int mouse_pos_x;
int mouse_pos_y;

Object* selecting_object;

std::vector<std::vector<Vec3>> screen_color(MAX_WIDTH, v_height);

Mesh FOCAL_PLANE;

void update_camera() {
    rt.update_size(WIDTH, HEIGHT);

    float tilted_a = camera->tilted_angle;
    float panned_a = camera->panned_angle;
    camera->init();
    camera->tilt(tilted_a);
    camera->pan(panned_a);

    rt.frame_count = 0;
}

void draw_to_window() {
    while(running) {
        if(rt.frame_count < rt.render_frame_count) {
            rt.draw_frame();
            screen_color = rt.screen_color;
        }
        else std::cout << "drawing thread resting ...\n";
        // for some unknown reasons that related to g++ optimization
        // there must be a line that always be called in order to keep this thread running
        // so the above line may seem useless and resource consumming
        // but it is necessary to keep the program run normally when using compiler optimization
        // TODO: think of a better way to run the draw thread
    }
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
    FOCAL_PLANE.update_material();
    FOCAL_PLANE.visible = false;
    FOCAL_PLANE.set_material(FOCAL_PLANE_MAT);
    rt.add_object(&FOCAL_PLANE);

    // cornell box

    Mesh plane = load_mesh_from("default_model/plane.obj");
    plane.set_scale({5, 5, 5});

    Material mat_red;
    mat_red.color = RED;
    mat_red.texture = new BaseTexture;
    Material mat_green;
    mat_green.color = GREEN;
    mat_green.texture = new BaseTexture;
    Material mat_white;
    mat_white.color = WHITE;
    mat_white.texture = new BaseTexture;
    Material mat_light;
    mat_light.color = WHITE;
    mat_light.emit_light = true;
    mat_light.emission_strength = 5.0f;
    mat_light.texture = new BaseTexture;

    Mesh floor = plane;
    floor.set_position({0, -5, 0});
    floor.set_material(mat_white);
    floor.update_material();
    rt.add_object(&floor);
    floor.calculate_AABB();

    Mesh ceil = plane;
    ceil.set_rotation({M_PI, 0, 0});
    ceil.set_position({0, 5, 0});
    ceil.set_material(mat_white);
    ceil.update_material();
    rt.add_object(&ceil);
    ceil.calculate_AABB();

    Mesh wall_back = plane;
    wall_back.set_rotation({M_PI/2, 0, 0});
    wall_back.set_position({0, 0, -5});
    wall_back.set_material(mat_white);
    wall_back.update_material();
    rt.add_object(&wall_back);
    wall_back.calculate_AABB();

    Mesh wall_red = plane;
    wall_red.set_rotation({0, 0, -M_PI/2});
    wall_red.set_position({-5, 0, 0});
    wall_red.set_material(mat_red);
    wall_red.update_material();
    rt.add_object(&wall_red);
    wall_red.calculate_AABB();

    Mesh wall_green = plane;
    wall_green.set_rotation({0, 0, M_PI/2});
    wall_green.set_position({5, 0, 0});
    wall_green.set_material(mat_green);
    wall_green.update_material();
    rt.add_object(&wall_green);
    wall_green.calculate_AABB();

    Mesh light = load_mesh_from("default_model/cube.obj");
    light.set_scale({2.5f, 0.1f, 2.5f});
    light.set_position({0, 5, 0});
    light.set_material(mat_light);
    light.update_material();
    rt.add_object(&light);
    light.calculate_AABB();

    // camera setting
    camera->position.z = 10;
    camera->focus_distance = 20.0f;
    camera->aperture = 0.2;
    camera->diverge_strength = 0.01;
    camera->FOV = 75.0f;
    camera->max_range = 100;
    camera->max_ray_bounce_count = 50;
    camera->ray_per_pixel = 1;

    camera->init();

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

                HitInfo hit = rt.get_collision_on(mouse_pos_x, mouse_pos_y);
                if(hit.did_hit) {
                    selecting_object = hit.object;
                }
                else selecting_object = nullptr;
            }
        }

        // handling keyboard signal
        if(camera_control) {
            float speed = SPEED * delta_time;
            float rot_speed = ROT_SPEED * delta_time;

            if(keys[SDL_SCANCODE_LSHIFT]) {
                speed *= 2;
            }
            if(keys[SDL_SCANCODE_LCTRL]) {
                speed /= 4;
                rot_speed /= 4;
            }

            Vec3 cam_pos = camera->position;
            float cam_pan = camera->panned_angle;
            float cam_tilt = camera->tilted_angle;

            if(keys[SDL_SCANCODE_LEFT]) camera->pan(rot_speed);
            if(keys[SDL_SCANCODE_RIGHT]) camera->pan(-rot_speed);
            if(keys[SDL_SCANCODE_UP]) camera->tilt(rot_speed);
            if(keys[SDL_SCANCODE_DOWN]) camera->tilt(-rot_speed);
            if(keys[SDL_SCANCODE_W]) camera->move_foward(speed);
            if(keys[SDL_SCANCODE_S]) camera->move_foward(-speed);
            if(keys[SDL_SCANCODE_A]) camera->move_right(-speed);
            if(keys[SDL_SCANCODE_D]) camera->move_right(speed);
            if(keys[SDL_SCANCODE_X]) camera->position.y += speed;
            if(keys[SDL_SCANCODE_Z]) camera->position.y -= speed;

            if(camera->position != cam_pos or camera->panned_angle != cam_pan or camera->tilted_angle != cam_tilt)
                rt.frame_count = 0;
        }

        float old_FOV = camera->FOV;
        float old_max_range = camera->max_range;
        float old_focus_distance = camera->focus_distance;

        int objs_state = 0;
        sdl.gui(
            &screen_color,
            &camera_control,
            &rt.lazy_ray_trace, &rt.render_frame_count, &rt.frame_count, rt.delay, rt.get_running_thread_count(),
            &WIDTH, &HEIGHT,
            &rt.objects, selecting_object, &objs_state,
            &rt.camera,
            &rt.up_sky_color, &rt.down_sky_color,
            &running
        );

        switch(objs_state) {
            case 1: // make new
                selecting_object = rt.objects.back();
                rt.frame_count = 0;
                break;
            case 2: // delete
                selecting_object = nullptr;
                rt.frame_count = 0;
                break;
        }

        if(old_FOV != camera->FOV
                or old_max_range != camera->max_range
                or old_focus_distance != camera->focus_distance
                or camera->WIDTH != WIDTH
                or camera->HEIGHT != HEIGHT)
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
