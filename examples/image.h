#ifndef IMAGE_H
#define IMAGE_H

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#endif

#include <vector>
#include "vec3.h"
#include "helper.h"

// save image from vector
inline void save_to_image(char* name, std::vector<std::vector<Vec3>>* colors, int tonemapping_method, float gamma, int WIDTH, int HEIGHT) {
    unsigned char data[WIDTH * HEIGHT * 3];

    for(int x = 0; x < WIDTH; x++)
        for(int y = 0; y < HEIGHT; y++) {
            Vec3 color = (*colors)[x][y];
            color = tonemap(color, tonemapping_method);
            color = gamma_correct(color, gamma);
            color *= 255;

            int r = color.x;
            int g = color.y;
            int b = color.z;

            unsigned char* pixel = data + (y * WIDTH + x) * 3;
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
        }
    stbi_write_png(name, WIDTH, HEIGHT, 3, data, WIDTH * 3);
}

unsigned char* load_image(const char* chr, int* image_width, int* image_height, int* channels) {
    return stbi_load(chr, image_width, image_height, channels, 0);
}

#endif
