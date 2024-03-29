#pragma once
#include "texture.h"

struct Material {
    Vec3 color = WHITE;
    float roughness = 1.0f;

    bool emit_light = false;
    float emission_strength = 0.0f;

    bool transparent = false;
    float refractive_index = RI_GLASS;

    bool smoke = false;
    float density = 0.5f;

    Texture* texture;
};
