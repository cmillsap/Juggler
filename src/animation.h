#pragma once

#include "scene.h"
#include "math_utils.h"
#include <vector>

class Animation {
public:
    // Constants from Java reference
    static constexpr double JUGGLE_X0 = -182.0;
    static constexpr double JUGGLE_X1 = -108.0;
    static constexpr double JUGGLE_Y0 = 88.0;
    static constexpr double JUGGLE_H_Y = 184.0;

    static constexpr double JUGGLE_H_VX = (JUGGLE_X0 - JUGGLE_X1) / 60.0;
    static constexpr double JUGGLE_L_VX = (JUGGLE_X1 - JUGGLE_X0) / 30.0;

    static constexpr double JUGGLE_H_H = JUGGLE_H_Y - JUGGLE_Y0;
    static constexpr double JUGGLE_H_VY = 4.0 * JUGGLE_H_H / 60.0;
    static constexpr double JUGGLE_G = JUGGLE_H_VY * JUGGLE_H_VY / (2.0 * JUGGLE_H_H);
    static constexpr double JUGGLE_L_VY = 0.5 * JUGGLE_G * 30.0;

    static constexpr double HIPS_MAX_Y = 85.0;
    static constexpr double HIPS_MIN_Y = 81.0;
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double HIPS_ANGLE_MULTIPLIER = 2.0 * PI / 30.0;

    static constexpr int FRAME_COUNT = 30;

    // Update all sphere positions for time T (0..30 cycle)
    void update(double T, std::vector<SphereData>& spheres);

private:
    void updateAppendage(
        std::vector<SphereData>& spheres, int baseIndex,
        const Vec3& p, const Vec3& q, const Vec3& normalAxis,
        double A, double B, int countA, int countB);
};
