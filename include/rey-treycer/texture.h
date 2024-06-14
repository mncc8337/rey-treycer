#ifndef TEXTURE_H
#define TEXTURE_H

#include <functional>

enum TEXTURE {
    TEX_NULL,
    TEX_COLOR,
    TEX_IMAGE,
    TEX_PROC,
};

struct SurfaceInfo {
    float u, v;
    Vec3 normal = VEC3_ZERO;
    Vec3 object_rotation = VEC3_ZERO;
};

class Texture {
public:
    virtual Vec3 get_texture(SurfaceInfo h) {
        return VEC3_ZERO;
    }
    virtual int get_type() {
        return TEX_NULL;
    }
};
// a place holder
class ColorTexture: public Texture {
public:
    Vec3 color = WHITE;
    Vec3 get_texture(SurfaceInfo h) {
        return color;
    }
    int get_type() {
        return TEX_COLOR;
    }
};

// a texture type that takes a image as texture
class ImageTexture: public Texture {
public:
    int channels;
    int image_width, image_height;
    unsigned char *pixel_data;

    // set the pizel_data, image_width/height and channels first before using this
    Vec3 get_texture(SurfaceInfo h) {
        int x = h.u * (image_width - 1);
        int y = h.v * (image_height - 1);

        unsigned char* pixel = pixel_data + (y * image_width + x) * channels;
        unsigned char r = pixel[0];
        unsigned char g = pixel[1];
        unsigned char b = pixel[2];

        return Vec3(r, g, b) / 255.0f;
    }
    int get_type() {
        return TEX_IMAGE;
    }
};

// a texture type that takes a function to generate texture
class ProceduralTexture: public Texture {
private:
    std::function<Vec3(SurfaceInfo)> func;
public:
    void set_function(std::function<Vec3(SurfaceInfo)> f) {
        // the function must take only a SurfaceInfo as param and return a Vec3 `color`
        // see predefined procedural textures at the end of `rey-treycer.h` for examples
        func = f;
    }
    Vec3 get_texture(SurfaceInfo h) {
        return func(h);
    }
    int get_type() {
        return TEX_PROC;
    }
};

#endif
