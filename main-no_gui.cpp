#include "rey-treycer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

int main() {
    ReyTreycer rt(1280, 720);

    // dice
    ImageTexture dice_tex;
    dice_tex.load_image("texture/dice.png");

    Material cube_mat;
    cube_mat.texture = &dice_tex;

    Mesh mesh = load_mesh_from("default_model/cube-uv.obj");
    mesh.set_material(cube_mat);
    mesh.update_material();
    rt.add_object(&mesh);

    ProceduralTexture normal_map_tex;
    normal_map_tex.set_function(&checker);
    Material sphere_mat;
    sphere_mat.texture = &normal_map_tex;

    Sphere sphere;
    sphere.set_material(sphere_mat);
    sphere.set_position({0, 5, 0});
    rt.add_object(&sphere);

    // camera setting
    Camera* camera = &rt.camera;
    camera->position.z = 10;
    camera->FOV = 90.0f;
    camera->max_range = 100;
    camera->max_ray_bounce_count = 50;
    camera->ray_per_pixel = 1;
    camera->init();

    while(rt.frame_count < rt.render_frame_count) {
        rt.draw_frame();
        std::cout << "frame " << rt.frame_count << " took " << rt.delay << " ms\n";
    }
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::string str;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
    str = "imgs/" + oss.str() + ".png";
    char *c = const_cast<char*>(str.c_str());
    save_to_image(c, &rt.screen_color, RGB_CLAMPING, 1, rt.WIDTH, rt.HEIGHT);

    return 0;
}