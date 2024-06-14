#include "rey-treycer.h"
#include "scenes.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#include "image.h"

int main(int argc, char** argv) {
    ReyTreycer rt(1280, 720);

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
    else all_textures(rt);

    // camera setting
    Camera* camera = &rt.camera;
    camera->position.z = 10;
    camera->FOV = 90.0f;
    camera->max_range = 100;
    camera->max_ray_bounce_count = 50;
    camera->ray_per_pixel = 1;
    camera->init();

    while(rt.rendered_count < 100) {
        auto start = std::chrono::system_clock::now();
        rt.draw_frame();
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        std::cout << "frame " << rt.rendered_count << " took " << (elapsed.count() * 1000) << " ms\n";
    }
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::string str;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y-%H-%M-%S");
    str = "imgs/" + oss.str() + ".png";
    char *c = const_cast<char*>(str.c_str());
    save_to_image(c, &rt.screen_color, TONEMAP_RGB_CLAMPING, 1, rt.WIDTH, rt.HEIGHT);

    return 0;
}
