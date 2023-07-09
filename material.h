#pragma once
#include "vec3.h"
#include "constant.h"
#include "texture.h"

struct Material {
    Vec3 color = WHITE;

    Vec3 emission_color = BLACK;
    float emission_strength = 0.0f;

    float roughness = 1.0f;
    float metal = 0.0f;

    Vec3 specular_color = WHITE;

    bool transparent = false;
    float refractive_index = RI_GLASS;

    bool smoke = false;
    float density = 0.5f;

    Texture texture;
};
