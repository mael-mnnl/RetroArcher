#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <cmath>
#include <algorithm>

namespace Core {
float rndF(float a, float b);
int   rndI(int a, int b);
float clampF(float v, float lo, float hi);
float distF(float ax, float ay, float bx, float by);
}

#endif
