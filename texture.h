#pragma once
#include "vec3.h"
#include "constant.h"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include <functional>
#include <iostream>

struct SurfaceInfo {
    float u, v;
    Vec3 normal = VEC3_ZERO;
    Vec3 object_rotation = VEC3_ZERO;
};

class Texture {
public:
    virtual Vec3 get_texture(SurfaceInfo h) {
        return VEC3_ZERO;
    };
    virtual bool has_texture() {
        return false;
    }
};
// a place holder
class BaseTexture: public Texture {
public:
    bool has_texture() {
        return false;
    }
    Vec3 get_texture(SurfaceInfo h) {
        return BLACK;
    }
};
class ImageTexture: public Texture {
private:
    int image_width, image_height;
    int channels;
    unsigned char *pixel_data;
public:
    bool has_texture() {
        return true;
    }
    void load_image(const char* chr) {
        pixel_data = stbi_load(chr, &image_width, &image_height, &channels, 0);
    }
    Vec3 get_texture(SurfaceInfo h) {
        int x = h.u * (image_width - 1);
        int y = h.v * (image_height - 1);

        unsigned char* pixel = pixel_data + (y * image_width + x) * channels;
        unsigned char r = pixel[0];
        unsigned char g = pixel[1];
        unsigned char b = pixel[2];

        return Vec3(r, g, b) / 255.0f;
    };
};
class ProceduralTexture: public Texture {
private:
    std::function<Vec3(SurfaceInfo)> func;
public:
    bool has_texture() {
        return true;
    }
    void set_function(std::function<Vec3(SurfaceInfo)> f) {
        func = f;
    }
    Vec3 get_texture(SurfaceInfo h) {
        return func(h);
    }
};
