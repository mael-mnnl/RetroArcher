#include "utils.h"
#include <QRandomGenerator>

namespace Core {
float rndF(float a, float b) {
    return a + QRandomGenerator::global()->generateDouble()*(b-a);
}
int rndI(int a, int b) {
    return QRandomGenerator::global()->bounded(a, b+1);
}
float clampF(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}
float distF(float ax, float ay, float bx, float by) {
    return std::hypot(bx-ax, by-ay);
}
}
