#pragma once
#include <vector>
#include "vec3.h"

const int MAX_WIDTH = 2000;
const int MAX_HEIGHT = 2000;

// how many threads in a row
const int row_threads = 2;
// how many threads in a column
const int column_threads = 4;

// other

const Vec3 BLACK(0, 0, 0);
const Vec3 WHITE(1, 1, 1);
const Vec3 RED(1, 0, 0);
const Vec3 GREEN(0, 1, 0);
const Vec3 BLUE(0, 0, 1);
const Vec3 YELLOW(1, 1, 0);
const Vec3 PURPLE(1, 0, 1);
const Vec3 CYAN(0, 1, 1);

const Vec3 VEC3_ZERO(0, 0, 0);

const float RI_AIR = 1.0003f;
const float RI_WATER = 1.333f;
const float RI_GLASS = 1.52f;
const float RI_FLINT_GLASS = 1.66f;
const float RI_DIAMOND = 2.4f;

const float pi = 3.141592654f;

const std::vector<Vec3> v_height(MAX_HEIGHT, VEC3_ZERO);
const std::vector<std::vector<Vec3>> v_screen(MAX_WIDTH, v_height);
