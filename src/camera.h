#pragma once

#include "math_utils.h"

struct Camera {
    Vec3 eye;
    Vec3 look;
    Vec3 u, v, w;          // ONB basis
    Vec3 virtualScreenCenter;
    double virtualScreenRatio;
    double distanceToScreen;
    double halfWidth;
    double halfHeight;

    void init(int screenWidth, int screenHeight);
    void updateOrbit(const Vec3& target, double angleRad, double radius, double height);
};
