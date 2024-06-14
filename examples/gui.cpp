#include <SDL2/SDL_events.h>
#include "rey-treycer.h"
#include "gui.h"
#include "scenes.h"

int WIDTH = 320;
int HEIGHT = 180;

ReyTreycer rt(WIDTH, HEIGHT, 8);
GUI gui(CHAR("ray tracer"), WIDTH, HEIGHT);
const Uint8 *keys = SDL_GetKeyboardState(NULL);

Camera* camera = &rt.camera;

const float SPEED = 5.0f;
const float ROT_SPEED = 0.8f;

bool running = true;
bool camera_control = true;

int mouse_pos_x;
int mouse_pos_y;

Object* selecting_object;

Mesh FOCAL_PLANE;

void restart_render() {
    rt.rendered_count = 0;
}

void update_camera() {
    rt.update_size(WIDTH, HEIGHT);

    float tilted_a = camera->tilted_angle;
    float panned_a = camera->panned_angle;
    camera->init();
    // after init camera with have default rotation so we need to re-rotate it
    camera->tilt(tilted_a);
    camera->pan(panned_a);

    restart_render();
}

int render_target = 100;

double delay = 0;
double total_delay = 0;
double avg_delay = 0;

bool drawing = false;
void draw_to_window() {
    auto start = std::chrono::system_clock::now();
    drawing = true;
    rt.draw_frame();
    drawing = false;
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    delay = elapsed.count() * 1000.0;

    // check if rt.rendered_count has increase a lot or not (jump)
    bool jumped = avg_delay * (rt.rendered_count - 1) - total_delay > 0.001;

    if(rt.rendered_count == 1) total_delay = 0;
    total_delay += delay;
    if(!jumped)
        avg_delay = total_delay / rt.rendered_count;
    else std::cout << "rendered count jumped, skip calculating delay\n";
}
std::thread drawing_thread;

void print_v3(Vec3 v) {
    std::cout << v.x << ' ' << v.y << ' ' << v.z << '\n';
}

float main_thread_delta_time = 0;
int main(int argc, char** argv) {
    ColorTexture focal_plane_tex;
    focal_plane_tex.color = Vec3(0.18, 0.5, 1.0);

    Material FOCAL_PLANE_MAT;
    FOCAL_PLANE_MAT.texture = &focal_plane_tex;
    FOCAL_PLANE_MAT.transparent = true;
    FOCAL_PLANE_MAT.refractive_index = RI_AIR;

    // add a plane to visualize focal length
    // it will be used in the gui app with the index 0
    Mesh FOCAL_PLANE = load_mesh_from("default_model/plane.obj");
    FOCAL_PLANE.visible = false;
    FOCAL_PLANE.set_material(FOCAL_PLANE_MAT);
    FOCAL_PLANE.update_material();
    rt.add_object(&FOCAL_PLANE);

    if(argc > 1) {
        if(std::string(argv[1]) == "cornell")
            cornell_box(rt);
        else if(std::string(argv[1]) == "textures")
            all_textures(rt);
        else if(std::string(argv[1]) == "all") {
            cornell_box(rt);
            all_textures(rt);
        }
        else {
            std::cout << "invalid arguement\n";
            return 1;
        }
    }
    else cornell_box(rt);

    // camera setting
    camera->position.z = 10;
    camera->focus_distance = 20.0f;
    camera->aperture = 0.2;
    camera->diverge_strength = 0.01;
    camera->FOV = 75.0f;
    camera->max_range = 100;
    camera->max_ray_bounce_count = 5;
    camera->ray_per_pixel = 1;

    camera->init();

    // start gui
    while(running) {
        auto start = std::chrono::system_clock::now();

        while(SDL_PollEvent(&(gui.event))) {
            SDL_GetMouseState(&mouse_pos_x, &mouse_pos_y);
            gui.process_gui_event();
            running = gui.event.type != SDL_QUIT;
            if(!running) break;

            // mouse click for selecting obj
            if(gui.event.type == SDL_MOUSEBUTTONDOWN and !gui.is_hover_over_gui()) {
                // convert to viewport position
                int w; int h;
                SDL_GetWindowSize(gui.window, &w, &h);
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
            float speed = SPEED * main_thread_delta_time;
            float rot_speed = ROT_SPEED * main_thread_delta_time;

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
            if(keys[SDL_SCANCODE_E]) camera->position.y += speed;
            if(keys[SDL_SCANCODE_Q]) camera->position.y -= speed;

            if(camera->position != cam_pos or camera->panned_angle != cam_pan or camera->tilted_angle != cam_tilt)
                restart_render();
        }

        float old_FOV = camera->FOV;
        float old_max_range = camera->max_range;
        float old_focus_distance = camera->focus_distance;

        int objs_state = 0;
        gui.gui(
            &(rt.screen_color),
            &camera_control,
            &rt.lazy_mode, &render_target, &rt.rendered_count,
            delay, avg_delay,
            rt.get_running_thread_count(),
            &WIDTH, &HEIGHT,
            &rt.objects, selecting_object, &objs_state,
            &rt.camera,
            &rt.up_sky_color, &rt.down_sky_color,
            &running
        );

        switch(objs_state) {
            case 1: // make new
                selecting_object = rt.objects.back();
                restart_render();
                break;
            case 2: // delete
                selecting_object = nullptr;
                restart_render();
                break;
        }

        if(old_FOV != camera->FOV
           or camera->WIDTH != WIDTH
           or camera->HEIGHT != HEIGHT)
            update_camera();
        else if(old_max_range != camera->max_range
                or old_focus_distance != camera->focus_distance)
            restart_render();

        gui.render();

        if(rt.rendered_count < render_target and !drawing) {
            // you need to join the old thread before start a new one
            if(main_thread_delta_time > 0) // if not first run
                drawing_thread.join();

            // start draw thread
            drawing_thread = std::thread(draw_to_window);
        }

        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        main_thread_delta_time = elapsed.count();
    }

    // wait for additional threads to end
    drawing_thread.join();

    gui.destroy();

    return 0;
}
