#pragma once
#include <SDL2/SDL.h>
#include "objects.h"
#include "camera.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

#include <vector>

#include <sstream>
#include <iomanip>

// TODO: add more backends

class SDL {
private:
    SDL_Texture* texture;
    ImGuiIO io;

    // prevously selected object
    Object* prev_object = nullptr;

    // object transform properties
    float position[3];
    float rotation[3];
    float scale[3];
    float radius;
    bool uniform_scaling = true;

    // object material properties
    float color[3];
    float emission_strength = 0.0f;
    bool emit_light = false;
    float roughness = 1.0f;
    bool transparent = false;
    float refractive_index = 0;
    const char* refractive_index_items[6] = {"air", "water", "glass", "flint glass", "diamond", "self-define"};
    int refractive_index_current_item = 1;
    bool smoke = false;
    float density = 1.0f;

    // editor settings
    bool show_crosshair = false;
    float up_sky_color[3] = {0.5, 0.7, 1.0};
    float down_sky_color[3] = {1.0, 1.0, 1.0};
    float gamma = 1.0f;

    // the focal plane for visualizing focus point
    Object* focal_plane = nullptr;
    bool show_focal_plane = false;

public:
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;

    // viewport geometries
    int WIDTH;
    int HEIGHT;

    // for SDL rendering
    unsigned char* pixels;
    int pitch;

    SDL(char* title, int w, int h) {
        WIDTH = w; HEIGHT = h;
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);

        window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                // default window geometries
                                1280, 720,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

        // setup imgui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = ImGui::GetIO(); (void)io;

        // so as not to burn your eyes
        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);
    }
    bool is_hover_over_gui() {
        return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    }
    // change viewport geometries
    void change_geometry(int w, int h) {
        WIDTH = w;
        HEIGHT = h;
        disable_drawing();
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    }
    // save image from vector
    void save_image(std::vector<std::vector<Vec3>>* screen_color, int tonemapping_method, float gamma) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::string str;
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
        str = "imgs/" + oss.str() + ".png";
        char *c = const_cast<char*>(str.c_str());

        save_to_image(c, screen_color, tonemapping_method, gamma, WIDTH, HEIGHT);
    }
    void process_gui_event() {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    // TODO: make the params look less ugly
    void gui(std::vector<std::vector<Vec3>>* screen,
             bool* camera_control,
             bool* lazy_ray_trace, int* frame_count, int* frame_num, double delay, int running_thread_count,
             int* width, int* height,
             std::vector<Object*>* oc, Object* selecting_object, int* objects_state,
             Camera* camera,
             Vec3* up_sky_c, Vec3* down_sky_c,
             bool* running) {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // copy all pixel to renderer
        enable_drawing();
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                // post processing
                Vec3 COLOR = tonemap((*screen)[x][y], RGB_CLAMPING);
                COLOR = gamma_correct(COLOR, gamma);

                draw_pixel(x, y, COLOR);
            }
        disable_drawing();

        // the UI part
        
        if(ImGui::CollapsingHeader("Editor")) {
            std::string info;
            if(*frame_num + 1 < *frame_count)
                info = "rendering frame " + std::to_string(*frame_num + 1) + '/' + std::to_string(*frame_count);
            else if(running_thread_count != 0) {
                info = "stopping, " + std::to_string(running_thread_count) + " thread(s) remain(s)";
            }
            else {
                info = "done rendering";
            }

            std::string delay_text = "last frame delay " + std::to_string(delay) + "ms";

            ImGui::Text("%s", info.c_str());
            ImGui::Text("%s", delay_text.c_str());

            ImGui::InputInt("viewport width", width, 1);
            *width = fmin(*width, MAX_WIDTH);
            *width = fmax(*width, 2);

            ImGui::InputInt("viewport height", height, 1);
            *height = fmin(*height, MAX_HEIGHT);
            *height = fmax(*height, 2);

            ImGui::Checkbox("camera control", camera_control);
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("turn on camera control\n WASD: position\n XZ: up/down\n right/left/up/down: angle");
            ImGui::Checkbox("lazy ray tracing", lazy_ray_trace);
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("increase performance but decrease image quality");

            ImGui::Checkbox("show crosshair", &show_crosshair);

            bool old_show_focal_plane = show_focal_plane;
            ImGui::Checkbox("show focal plane", &show_focal_plane);
            if(show_focal_plane) {
                // add focal plane if not have
                if(focal_plane == nullptr) {
                    focal_plane = (*oc)[0];
                }
                // show the focal plane
                focal_plane->visible = true;

                Vec3 new_pos = camera->position + camera->get_looking_direction() * camera->focus_distance;
                Vec3 rotating_axis = -camera->get_right_direction();

                focal_plane->tris = focal_plane->default_tris;
                for(int i = 0; i < (int)focal_plane->tris.size(); i++) {
                    for(int j = 0; j < 3; j++) {
                        Vec3* pos = &(focal_plane->tris[i].vert[j]);
                        *pos = _scale(*pos, {1e3, 0, 1e3});
                        *pos = _rotate_y(*pos, camera->panned_angle); // panned angle
                        *pos = _rotate_on_axis(*pos, rotating_axis, camera->tilted_angle + M_PI / 2); // tilted angle
                        *pos = *pos + new_pos;
                    }
                }
                // calculate AABB for rendering
                focal_plane->calculate_AABB();
            }
            else if(focal_plane != nullptr) {
                // hide the focal plane object
                focal_plane->visible = false;
            }
            // force render if clicked
            if(old_show_focal_plane != show_focal_plane)
                *frame_num = 0;

            ImGui::ColorEdit3("up sky color", up_sky_color);
            Vec3 ukc = Vec3(up_sky_color[0], up_sky_color[1], up_sky_color[2]);
            if(*up_sky_c != ukc)
                *up_sky_c = ukc;

            ImGui::ColorEdit3("down sky color", down_sky_color);
            Vec3 dkc = Vec3(down_sky_color[0], down_sky_color[1], down_sky_color[2]);
            if(*down_sky_c != dkc)
                *down_sky_c = dkc;

            ImGui::InputInt("frame count", frame_count, 1);
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("number of frame will be rendered");

            if(ImGui::Button("render"))
                *frame_num = 0;
            ImGui::SameLine();
            if(ImGui::Button("stop render"))
                *frame_num = *frame_count;

            if(ImGui::Button("fit window size with viewport size")) SDL_SetWindowSize(window, *width, *height);
            if(ImGui::Button("save image")) save_image(screen, RGB_CLAMPING, gamma);
            ImGui::SameLine();
            if(ImGui::Button("quit"))
                *running = false;
        }

        if(ImGui::CollapsingHeader("camera")) {
            ImGui::DragFloat("gamma correction", &gamma, 0.01f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::SliderFloat("FOV", &(camera->FOV), 1.0f, 179.0f);
            ImGui::DragFloat("focus distance", &(camera->focus_distance), 0.1f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("aperture", &(camera->aperture), 0.001f, 0, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("diverge strength", &(camera->diverge_strength), 0.1f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("max range", &(camera->max_range), 1, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::InputInt("max ray bounce", &(camera->max_ray_bounce_count), 1);
            camera->max_ray_bounce_count = fmax(camera->max_ray_bounce_count, 1);

            ImGui::InputInt("ray per pixel", &(camera->ray_per_pixel), 1);
            camera->ray_per_pixel = fmax(camera->ray_per_pixel, 1);
        }

        ImGui::Begin("object property");
        if(selecting_object == nullptr) {
            bool new_obj = false;
            if(ImGui::Button("add sphere")) {
                Material mat;
                mat.texture = new BaseTexture;

                Sphere* sphere = new Sphere;
                sphere->set_material(mat);
                oc->push_back(sphere);
                new_obj = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("add plane")) {
                Material mat;
                mat.texture = new BaseTexture;

                Mesh* mesh = new Mesh;
                *mesh = load_mesh_from("default_model/plane.obj");
                mesh->update_material();
                mesh->set_material(mat);
                oc->push_back(mesh);
                new_obj = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("add cube")) {
                Material mat;
                mat.texture = new BaseTexture;

                Mesh* mesh = new Mesh;
                *mesh = load_mesh_from("default_model/cube.obj");
                mesh->update_material();
                mesh->set_material(mat);
                oc->push_back(mesh);
                new_obj = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("add dodecahedron")) {
                Material mat;
                mat.texture = new BaseTexture;

                Mesh* mesh = new Mesh;
                *mesh = load_mesh_from("default_model/dodecahedron.obj");
                mesh->update_material();
                mesh->set_material(mat);
                oc->push_back(mesh);
                new_obj = true;
            }
            *objects_state = new_obj;
        }
        else if(selecting_object == focal_plane) {
            ImGui::Text("selecting focal plane");
        }
        else {
            const char* typ;
            if(selecting_object->is_sphere())
                typ = std::string("sphere").c_str();
            else
                typ = std::string("mesh").c_str();
            ImGui::Text("selecting a %s", typ);

            // if select a new object
            if(prev_object != selecting_object) {
                prev_object = selecting_object;

                Vec3 pos = selecting_object->get_position();
                Vec3 rot = selecting_object->get_rotation();
                Vec3 scl = selecting_object->get_scale();
                float rad = selecting_object->get_radius();
                Material mat = selecting_object->get_material();

                position[0] = pos.x;
                position[1] = pos.y;
                position[2] = pos.z;

                rotation[0] = rad2deg(rot.x);
                rotation[1] = rad2deg(rot.y);
                rotation[2] = rad2deg(rot.z);

                scale[0] = scl.x;
                scale[1] = scl.y;
                scale[2] = scl.z;

                uniform_scaling = scl.x == scl.y and scl.y == scl.z;

                radius = rad;
                
                color[0] = mat.color.x;
                color[1] = mat.color.y;
                color[2] = mat.color.z;


                emit_light = mat.emit_light;
                emission_strength = mat.emission_strength;

                roughness = mat.roughness;

                transparent = mat.transparent;
                refractive_index = mat.refractive_index;

                smoke = mat.smoke;
                density = mat.density;
            }

            ImGui::Text("transform");
            ImGui::DragFloat3("position", position, 0.1f);
            ImGui::DragFloat3("rotation", rotation, 1.0f);
            if(selecting_object->is_sphere()) {
                ImGui::DragFloat("radius", &radius, 0.5f);
            }
            else {
                ImGui::DragFloat3("scaling", scale, 0.1f, 0.001f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                ImGui::Checkbox("uniform scaling", &uniform_scaling);
            }
            ImGui::Text("material");
            ImGui::ColorEdit3("color", color);
            ImGui::Checkbox("emit light", &emit_light);
            if(emit_light)
                ImGui::DragFloat("emission_strength", &emission_strength, 0.1f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("roughness", &roughness, 0, 1);
            ImGui::Checkbox("transparent", &transparent);
            if(transparent) {
                ImGui::Combo("refractive index", &refractive_index_current_item, refractive_index_items, 6);
                switch(refractive_index_current_item) {
                    case 0:
                        refractive_index = RI_AIR;
                        break;
                    case 1:
                        refractive_index = RI_WATER;
                        break;
                    case 2:
                        refractive_index = RI_GLASS;
                        break;
                    case 3:
                        refractive_index = RI_FLINT_GLASS;
                        break;
                    case 4:
                        refractive_index = RI_DIAMOND;
                        break;
                    case 5:
                        ImGui::InputFloat(" ", &refractive_index, 0.01f);
                        break;
                }
            }
            ImGui::Checkbox("smoke", &smoke);
            if(smoke)
                ImGui::DragFloat("smoke density", &density, 0.001f, 0.00001f, INFINITY, "%.5f", ImGuiSliderFlags_AlwaysClamp);

            if(uniform_scaling) {
                int difference_count = (scale[0] != scale[1]) + (scale[1] != scale[2]) + (scale[0] != scale[2]);
                float MAX = fmax(scale[0], fmax(scale[1], scale[2]));
                float MIN = fmin(scale[0], fmin(scale[1], scale[2]));
                int max_count = 0;
                for(int i = 0; i < 3; i++)
                    if(scale[i] == MAX) max_count++;

                if(difference_count == 3) {
                    scale[0] = MAX;
                    scale[1] = MAX;
                    scale[2] = MAX;
                }
                else {
                    if(max_count == 2) {
                        scale[0] = MIN;
                        scale[1] = MIN;
                        scale[2] = MIN;
                    }
                    else {
                        scale[0] = MAX;
                        scale[1] = MAX;
                        scale[2] = MAX;
                    }
                }
            }

            Vec3 new_pos = Vec3(position[0], position[1], position[2]);
            Vec3 new_rot = Vec3(deg2rad(rotation[0]), deg2rad(rotation[1]), deg2rad(rotation[2]));
            Vec3 new_scl = Vec3(scale[0], scale[1], scale[2]);
            Vec3 new_color = Vec3(color[0], color[1], color[2]);

            Object* obj = selecting_object;
            Material mat = obj->get_material();
            bool scale_changed;
            if(obj->is_sphere())
                scale_changed = obj->get_radius() != radius;
            else
                scale_changed = obj->get_scale() != new_scl;

            bool object_changed = obj->get_position() != new_pos
                                    or obj->get_rotation() != new_rot
                                    or scale_changed
                                    or mat.color != new_color
                                    or mat.emit_light != emit_light
                                    or mat.emission_strength != emission_strength
                                    or mat.roughness != roughness
                                    or mat.transparent != transparent
                                    or mat.refractive_index != refractive_index
                                    or mat.smoke != smoke
                                    or mat.density != density;
            if(object_changed) {
                if(obj->is_sphere())
                    obj->set_radius(radius);
                else
                    obj->set_scale(new_scl);
                obj->set_rotation(new_rot);
                obj->set_position(new_pos);
                mat.color = new_color;
                mat.emit_light = emit_light;
                mat.emission_strength = emission_strength;
                mat.roughness = roughness;
                mat.transparent = transparent;
                mat.refractive_index = refractive_index;
                mat.smoke = smoke;
                mat.density = density;
                obj->set_material(mat);
                obj->calculate_AABB();
                *frame_num = 0;
            }
            if(ImGui::Button("delete object")) {
                for(int i = 0; i < (int)oc->size(); i++)
                    if((*oc)[i] == selecting_object)
                        oc->erase(oc->begin() + i);

                *objects_state = 2;
            }
        }
        ImGui::End();

        // up date viewport geometries if changed
        if(*width != WIDTH or *height != HEIGHT) change_geometry(*width, *height);
    }

    // graphics
    void enable_drawing() {
        SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
    }
    void draw_pixel(int x, int y, Vec3 color) {
        pixels[y * pitch + x * 4 + 0] = color.z * 255;
        pixels[y * pitch + x * 4 + 1] = color.y * 255;
        pixels[y * pitch + x * 4 + 2] = color.x * 255;
        pixels[y * pitch + x * 4 + 3] = 255;
    }
    void disable_drawing() {
        SDL_UnlockTexture(texture);
    }
    void load_texture() {
        // scale the texture to fit the window
        SDL_Rect rect;
        rect.x = 0; rect.y = 0;
        SDL_GetWindowSize(window, &(rect.w), &(rect.h));
        // load texture to renderer
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
    // over-complicated maths to draw a crosshair in the middle of the window
    void draw_crosshair() {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        int middle_x = w >> 1; // divided by 2 but faster
        int middle_y = h >> 1; // divided by 2 but faster
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        // black magic
        for(int i = -!(w % 2); i <= 0; i++)
            SDL_RenderDrawLine(renderer, middle_x + i, middle_y - 5, middle_x + i, middle_y + 5 - !(h % 2));
        for(int i = -!(h % 2); i <= 0; i++)
            SDL_RenderDrawLine(renderer, middle_x - 5, middle_y + i, middle_x + 5 - !(w % 2), middle_y + i);
    }
    void render() {
        load_texture();
        if(show_crosshair) draw_crosshair();

        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
    void destroy() {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};
