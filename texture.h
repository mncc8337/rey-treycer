#pragma once
#include "vec3.h"
#include "constant.h"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include <iostream>

class Texture {
    int image_width, image_height;
    int channels;
    unsigned char *pixel_data;
public:
    bool image_texture = false;
    bool sphere_texture = false;

    void load_image(const char* chr) {
        image_texture = true;
        pixel_data = stbi_load(chr, &image_width, &image_height, &channels, 3);
    }
    Vec3 get_sphere_texture(float u, float v) {
        int x = u * (image_width - 1);
        int y = v * (image_height - 1);

        unsigned char* pixel = pixel_data + (y * image_width + x) * channels;
        unsigned char r = pixel[0];
        unsigned char g = pixel[1];
        unsigned char b = pixel[2];

        return Vec3(r, g, b) / 255.0f; // (normal + Vec3(1, 1, 1)) / 2
    };
};
