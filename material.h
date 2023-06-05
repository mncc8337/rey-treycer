#pragma once
#include "vec3.h"
#include "constant.h"

struct Material {
    Vec3 color = WHITE;

    Vec3 emission_color = BLACK;
    float emission_strength = 0.0f;

    float roughness = 1.0f;
    float metal = 1.0f;

    bool transparent = false;
    float refractive_index = RI_GLASS;
};
