#pragma once
#include <vector>
#include "vec3.h"

// renderer

const int WIDTH = 512; // 512
const int HEIGHT = 300; // 300

// how many threads in a row
const int row_threads = 2;
// how many threads in a column
const int column_threads = 4;

const bool record = true;

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

const float RI_AIR = 1;
const float RI_WATER = 1.33f;
const float RI_GLASS = 1.52f;
const float RI_DENSE_GLASS = 1.66f;
const float RI_DIAMOND = 2.4f;

const float pi = 3.141592654f;

const std::vector<Vec3> v_height(HEIGHT, VEC3_ZERO);
const std::vector<std::vector<Vec3>> v_screen(WIDTH, v_height);
