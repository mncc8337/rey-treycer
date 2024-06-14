#ifndef MATERIAL_H
#define MATERIAL_H

#include "texture.h"

struct Material {
    float roughness = 1.0f;

    bool emit_light = false;
    float emission_strength = 0.0f;

    bool transparent = false;
    float refractive_index = RI_GLASS;

    bool smoke = false;
    float density = 0.5f;

    Texture* texture;
};

#endif
