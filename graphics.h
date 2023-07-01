#pragma once
#include <SDL2/SDL.h>
#include "vec3.h"
#include "constant.h"
#include "objects.h"
#include "camera.h"
#include "transformation.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "CImg.h"

using namespace cimg_library;

class SDL {
private:
    std::vector<std::vector<Vec3>> screen_color;
    SDL_Texture* texture;
    ImGuiIO io;

    int prev_object_id = -1;
    int prev_object_type;

    // object transform property
    float position[3];
    float rotation[3];
    float scale[3];
    float radius;
    bool uniform_scaling = true;

    // object material property
    float color[3];
    float emission_color[3];
    float emission_strength = 0.0f;
    float roughness = 1;
    bool transparent = false;
    float refractive_index = 0;
    const char* refractive_index_items[6] = {"air", "water", "glass", "flint glass", "diamond", "self-define"};
    int refractive_index_current_item = 1;

    // editor setting
    bool show_crosshair = false;
    float up_sky_color[3] = {0.5, 0.7, 1.0};
    float down_sky_color[3] = {1.0, 1.0, 1.0};

    int focal_plane_id = -5;
    bool show_focal_plane = false;
    bool focal_plane_spawned = false;

public:
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;

    int WIDTH;
    int HEIGHT;

    unsigned char* pixels;
    int pitch;

    SDL(char* title, int w, int h) {
        WIDTH = w; HEIGHT = h;
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);

        window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                1280, 720,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

        screen_color = std::vector<std::vector<Vec3>>(MAX_WIDTH, v_height);

        // setup imgui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);
    }
    bool is_hover_over_gui() {
        return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    }
    void change_geometry(int w, int h) {
        WIDTH = w;
        HEIGHT = h;
        disable_drawing();
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    }
    void save_image() {
        CImg<int> image(WIDTH, HEIGHT, 1, 3);
        for(int x = 0; x < WIDTH; x++)
            for(int y = 0; y < HEIGHT; y++) {
                Vec3 c = screen_color[x][y];
                int COLOR[] = {int(c.x * 255), int(c.y * 255), int(c.z * 255)};
                image.draw_point(x, y, COLOR, 1.0f);
            }
        
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::string str;
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
        str = "res/" + oss.str() + ".bmp";
        char *c = const_cast<char*>(str.c_str());

        image.save(c);
    }
    void process_gui_event() {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    void gui(bool* lazy_ray_trace, int* frame_count, int* frame_num, double delay, int* width, int* height, ObjectContainer* oc, int* selecting_object, int* selecting_object_type, Camera* camera, float* gamma_correction, Vec3* up_sky_c, Vec3* down_sky_c, bool* running) {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        if(ImGui::CollapsingHeader("Editor")) {
            std::string info = "done rendering";
            if(*frame_num + 1 < *frame_count)
                info = "rendering frame " + std::to_string(*frame_num + 1) + '/' + std::to_string(*frame_count);

            std::string delay_text = "last frame delay " + std::to_string(delay) + "ms";

            ImGui::Text("%s", info.c_str());
            ImGui::Text("%s", delay_text.c_str());

            ImGui::InputInt("viewport width", width, 1);
            *width = fmin(*width, MAX_WIDTH);
            *width = fmax(*width, 2);

            ImGui::InputInt("viewport height", height, 1);
            *height = fmin(*height, MAX_HEIGHT);
            *height = fmax(*height, 2);

            ImGui::Checkbox("lazy ray tracing", lazy_ray_trace);
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("increase performance but decrease image quality");

            ImGui::Checkbox("show crosshair", &show_crosshair);

            bool old_show_focal_plane = show_focal_plane;
            ImGui::Checkbox("show focal plane", &show_focal_plane);
            if(show_focal_plane) {
                // add focal plane mesh if not have
                if(!focal_plane_spawned) {
                    focal_plane_id = oc->add_mesh(load_mesh_from("default_model/plane.obj"));
                    focal_plane_spawned = true;
                }
                // "show" the focal plane
                oc->meshes_available[focal_plane_id] = true;

                Material mat;
                mat.color = BLACK;
                mat.emission_color = WHITE;
                mat.emission_strength = 0.1f;

                Mesh* focal_plane = &(oc->meshes[focal_plane_id]);
                Vec3 look_dir = camera->get_looking_direction();
                Vec3 new_pos = camera->position + look_dir * camera->focal_length;
                // get perpendicular vector of camera looking dir
                Vec3 rotating_axis = look_dir.cross({0, 1, 0}).normalize();

                focal_plane->tris = focal_plane->default_tris;
                for(int i = 0; i < (int)focal_plane->tris.size(); i++) {
                    for(int j = 0; j < 3; j++) {
                        Vec3* pos = &(focal_plane->tris[i].vert[j]);
                        *pos = _scale(*pos, {1e3, 0, 1e3});
                        *pos = _rotate_y(*pos, camera->panned_angle); // panned angle
                        *pos = _rotate_on_axis(*pos, rotating_axis, camera->tilted_angle + M_PI / 2); // tilted angle
                        *pos = *pos + new_pos;
                    }
                    focal_plane->tris[i].material = mat;
                }
                // calculate AABB for rendering
                focal_plane->calculate_AABB();
            }
            else {
                // "hide" the focal plane object
                oc->meshes_available[focal_plane_id] = false;
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
            if(ImGui::Button("save image")) save_image();
            ImGui::SameLine();
            if(ImGui::Button("quit"))
                *running = false;
        }

        if(ImGui::CollapsingHeader("camera")) {
            ImGui::DragFloat("gamma correction", gamma_correction, 0.01f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::SliderFloat("FOV", &(camera->FOV), 0, 180);
            ImGui::DragFloat("focal length", &(camera->focal_length), 0.1f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("max range", &(camera->max_range), 1, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("blur rate", &(camera->blur_rate), 0.001f, 0.0f, INFINITY, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::InputInt("max ray bounce", &(camera->max_ray_bounce_count), 1);
            camera->max_ray_bounce_count = fmax(camera->max_ray_bounce_count, 1);

            ImGui::InputInt("ray per pixel", &(camera->ray_per_pixel), 1);
            camera->ray_per_pixel = fmax(camera->ray_per_pixel, 1);
        }

        ImGui::Begin("object property");
        if(*selecting_object == -1) {
            int id = -1;
            if(ImGui::Button("add sphere")) {
                Sphere sphere;
                id = oc->add_sphere(sphere);
                *selecting_object = id;
                *selecting_object_type = TYPE_SPHERE;
            }
            ImGui::SameLine();
            if(ImGui::Button("add plane")) {
                Mesh mesh = load_mesh_from("default_model/plane.obj");
                id = oc->add_mesh(mesh);
                *selecting_object = id;
                *selecting_object_type = TYPE_MESH;
            }
            ImGui::SameLine();
            if(ImGui::Button("add cube")) {
                Mesh mesh = load_mesh_from("default_model/cube.obj");
                id = oc->add_mesh(mesh);
                *selecting_object = id;
                *selecting_object_type = TYPE_MESH;
            }
            ImGui::SameLine();
            if(ImGui::Button("add dodecahedron")) {
                Mesh mesh = load_mesh_from("default_model/dodecahedron.obj");
                id = oc->add_mesh(mesh);
                *selecting_object = id;
                *selecting_object_type = TYPE_MESH;
            }
            if(id != -1)
                *frame_num = 0;
        }
        else if(*selecting_object == focal_plane_id and *selecting_object_type == TYPE_MESH) {
            ImGui::Text("selecting focal plane");
        }
        else {
            const char* typ;
            if(*selecting_object_type == TYPE_SPHERE)
                typ = std::string("sphere").c_str();
            else
                typ = std::string("mesh").c_str();
            ImGui::Text("selecting %s %d", typ, *selecting_object);

            // if select a new object
            if(prev_object_id != *selecting_object or prev_object_type != *selecting_object_type) {
                if(*selecting_object_type == TYPE_SPHERE) {
                    Vec3 centre = oc->spheres[*selecting_object].centre;
                    Material mat = oc->spheres[*selecting_object].material;

                    position[0] = centre.x;
                    position[1] = centre.y;
                    position[2] = centre.z;

                    rotation[0] = rad2deg(mat.texture.texture_rotation.x);
                    rotation[1] = rad2deg(mat.texture.texture_rotation.y);
                    rotation[2] = rad2deg(mat.texture.texture_rotation.z);

                    radius = oc->spheres[*selecting_object].radius;
                    color[0] = mat.color.x;
                    color[1] = mat.color.y;
                    color[2] = mat.color.z;

                    emission_color[0] = mat.emission_color.x;
                    emission_color[1] = mat.emission_color.y;
                    emission_color[2] = mat.emission_color.z;
                    emission_strength = mat.emission_strength;

                    roughness = mat.roughness;
                    transparent = mat.transparent;
                    refractive_index = mat.refractive_index;
                }
                else {
                    Vec3 pos = oc->meshes[*selecting_object].get_position();
                    Vec3 rot = oc->meshes[*selecting_object].get_rotation();
                    Vec3 scl = oc->meshes[*selecting_object].get_scale();
                    Material mat = oc->meshes[*selecting_object].get_material();

                    position[0] = pos.x;
                    position[1] = pos.y;
                    position[2] = pos.z;

                    rotation[0] = rad2deg(rot.x);
                    rotation[1] = rad2deg(rot.y);
                    rotation[2] = rad2deg(rot.z);

                    scale[0] = scl.x;
                    scale[1] = scl.y;
                    scale[2] = scl.z;
                    
                    color[0] = mat.color.x;
                    color[1] = mat.color.y;
                    color[2] = mat.color.z;

                    emission_color[0] = mat.emission_color.x;
                    emission_color[1] = mat.emission_color.y;
                    emission_color[2] = mat.emission_color.z;
                    emission_strength = mat.emission_strength;

                    roughness = mat.roughness;
                    transparent = mat.transparent;
                    refractive_index = mat.refractive_index;
                }
            }
            prev_object_id = *selecting_object;
            prev_object_type = *selecting_object_type;

            ImGui::Text("transform");
            ImGui::DragFloat3("position", position, 0.1f);
            ImGui::DragFloat3("rotation", rotation, 1.0f);
            if(*selecting_object_type == TYPE_SPHERE) {
                ImGui::DragFloat("radius", &radius, 0.5f);
            }
            else {
                ImGui::DragFloat3("scaling", scale, 0.1f);
                ImGui::Checkbox("uniform scaling", &uniform_scaling);
            }
            ImGui::Text("material");
            ImGui::ColorEdit3("color", color);
            ImGui::ColorEdit3("emission color", emission_color);
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
            Vec3 new_emission_color = Vec3(emission_color[0], emission_color[1], emission_color[2]);

            if(*selecting_object_type == TYPE_SPHERE) {
                Sphere* sphere = &(oc->spheres[*selecting_object]);
                Material* mat = &(sphere->material);
                bool object_changed = sphere->centre != new_pos
                                      or sphere->radius != radius
                                      or mat->color != new_color
                                      or mat->emission_color != new_emission_color
                                      or mat->emission_strength != emission_strength
                                      or mat->roughness != roughness
                                      or mat->transparent != transparent
                                      or mat->refractive_index != refractive_index
                                      or mat->texture.texture_rotation != new_rot;
                if(object_changed) {
                    sphere->centre = new_pos;
                    sphere->radius = radius;
                    mat->color = new_color;
                    mat->emission_color = new_emission_color;
                    mat->emission_strength = emission_strength;
                    mat->roughness = roughness;
                    mat->transparent = transparent;
                    mat->refractive_index = refractive_index;
                    mat->texture.texture_rotation = new_rot;
                    *frame_num = 0;
                }
            }
            else {
                Mesh* mesh = &(oc->meshes[*selecting_object]);
                Material mat = mesh->get_material();
                bool object_changed = mesh->get_position() != new_pos
                                      or mesh->get_rotation() != new_rot
                                      or mesh->get_scale() != new_scl
                                      or mat.color != new_color
                                      or mat.emission_color != new_emission_color
                                      or mat.emission_strength != emission_strength
                                      or mat.roughness != roughness
                                      or mat.transparent != transparent
                                      or mat.refractive_index != refractive_index;
                if(object_changed) {
                    mesh->set_scale(new_scl);
                    mesh->set_rotation(new_rot);
                    mesh->set_position(new_pos);
                    mat.color = new_color;
                    mat.emission_color = new_emission_color;
                    mat.emission_strength = emission_strength;
                    mat.roughness = roughness;
                    mat.transparent = transparent;
                    mat.refractive_index = refractive_index;
                    mesh->set_material(mat);
                    mesh->calculate_AABB();
                    *frame_num = 0;
                }
            }
            if(ImGui::Button("delete object")) {
                if(*selecting_object_type == TYPE_SPHERE)
                    oc->remove_sphere(*selecting_object);
                else
                    oc->remove_mesh(*selecting_object);

                *selecting_object = -1;
                *frame_num = 0;
            }
        }
        ImGui::End();

        if(*width != WIDTH or *height != HEIGHT) change_geometry(*width, *height);
    }
    // graphics
    void enable_drawing() {
        SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
    }
    void draw_pixel(int x, int y, Vec3 color) {
        screen_color[x][y] = color;
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
    void draw_crosshair() {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        int middle_x = w >> 1;
        int middle_y = h >> 1;
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
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
