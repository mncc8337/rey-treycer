#ifndef MATERIAL_H
#define MATERIAL_H

#include "texture.h"

struct Material {
    float roughness = 1.0f;
    float emission_strength = 0.0f;
    float ior = -1.0; // set to <0.0 to disable
    float volume_density = 1.0f; // set to >1.0 to disable
    Texture* texture;
};

#endif
