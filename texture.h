#pragma once
#include "vec3.h"
#include "constant.h"
#include "transformation.h"
#include <SDL2/SDL_image.h>

#include <iostream>

class Texture {
    SDL_Surface* image;
public:
    bool image_texture = false;
    bool sphere_texture = false;
    
    Vec3 texture_rotation = VEC3_ZERO; // for sphere only

    void load_image(const char* chr) {
        image_texture = true;
        image = IMG_Load(chr);
    }
    Vec3 get_sphere_texture(Vec3 p) {
        p = _rotate(p, texture_rotation);
        float theta = acos(p.y);
        float phi = atan2(p.z, p.x) + M_PI;
        float u = phi / (2 * M_PI);
        float v = theta / M_PI;
        int x = u * (image->w - 1);
        int y = v * (image->h - 1);

        Uint8* pixel = (Uint8*)image->pixels + y * image->pitch + x * (int)image->format->BytesPerPixel;
        Uint8 r, g, b;
        SDL_GetRGB(*(Uint32*)pixel, image->format, &r, &g, &b);

        return Vec3(r / 255.0f, g / 255.0f, b / 255.0f);
    };
};
