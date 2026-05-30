#include "animation.h"
#include <cmath>
#include <stdexcept>

void Animation::update(double T, std::vector<SphereData>& spheres) {
    // Ball 1 (sphere index 1): high throw, launched 30 frames ago
    {
        double t = 30.0 + T;
        spheres[1].center.z = JUGGLE_X1 + JUGGLE_H_VX * t;
        spheres[1].center.y = JUGGLE_Y0 + (JUGGLE_H_VY - 0.5 * JUGGLE_G * t) * t;
    }

    // Ball 2 (sphere index 2): high throw, launched at T=0
    {
        double t = T;
        spheres[2].center.z = JUGGLE_X1 + JUGGLE_H_VX * t;
        spheres[2].center.y = JUGGLE_Y0 + (JUGGLE_H_VY - 0.5 * JUGGLE_G * t) * t;
    }

    // Ball 0 (sphere index 0): low throw
    {
        double t = T;
        spheres[0].center.z = JUGGLE_X0 + JUGGLE_L_VX * t;
        spheres[0].center.y = JUGGLE_Y0 + (JUGGLE_L_VY - 0.5 * JUGGLE_G * t) * t;
    }

    // Body oscillation
    double angle = HIPS_ANGLE_MULTIPLIER * T;
    double oscillation = 0.5 * (1.0 + std::cos(angle));

    Vec3 o = { 151.0, HIPS_MIN_Y + (HIPS_MAX_Y - HIPS_MIN_Y) * oscillation, -151.0 };

    // Body tilt vectors
    Vec3 bodyV = { 0.0, 70.0, (HIPS_MIN_Y - HIPS_MAX_Y) * std::sin(angle) };
    bodyV.normalize();

    Vec3 bodyU = { 0.0, bodyV.z, -bodyV.y };
    Vec3 bodyW = { 1.0, 0.0, 0.0 };

    // Torso spheres (indices 3..10)
    for (int i = 0; i < 8; i++) {
        double percent = i / 7.0;
        spheres[3 + i].center = Vec3::ray(o, bodyV, 32.0 * percent);
    }

    // Head (index 11)
    spheres[11].center = Vec3::ray(o, bodyV, 70.0);

    // Neck (index 12)
    spheres[12].center = Vec3::ray(o, bodyV, 55.0);

    // Left leg (indices 13..29)
    {
        Vec3 p = { 159.0, 2.5, -133.0 };
        Vec3 q = Vec3::mapYZ(o, bodyV, bodyU, -9.0, -16.0);
        updateAppendage(spheres, 13, p, q, bodyU, 42.58, 34.07, 8, 8);
    }

    // Right leg (indices 30..46)
    {
        Vec3 p = { 139.0, 2.5, -164.0 };
        Vec3 q = Vec3::mapYZ(o, bodyV, bodyU, -9.0, 16.0);
        updateAppendage(spheres, 30, p, q, bodyU, 42.58, 34.07, 8, 8);
    }

    // Left arm (indices 47..63)
    {
        double armAngle = -0.35 * oscillation;
        Vec3 p = { 69.0 + 41.0 * std::cos(armAngle),
                   60.0 - 41.0 * std::sin(armAngle),
                   -108.0 };
        Vec3 q = Vec3::mapYZ(o, bodyV, bodyU, 45.0, -19.0);
        Vec3 n = Vec3::mapYZ(o, bodyV, bodyU, 45.41217, -19.91111);
        n = n - q;
        updateAppendage(spheres, 47, p, q, n, 44.294, 46.098, 8, 8);
    }

    // Right arm (indices 64..80)
    {
        double armAngle = -0.35 * oscillation;
        Vec3 p = { 69.0 + 41.0 * std::cos(armAngle),
                   60.0 - 41.0 * std::sin(armAngle),
                   -182.0 };
        Vec3 q = Vec3::mapYZ(o, bodyV, bodyU, 45.0, 19.0);
        Vec3 n = Vec3::mapYZ(o, bodyV, bodyU, 45.41217, 19.91111);
        n = q - n;
        updateAppendage(spheres, 64, p, q, n, 44.294, 46.098, 8, 8);
    }

    // Left eye (index 81)
    spheres[81].center = Vec3::mapYZ(o, bodyV, bodyU, 69.0, -7.0);
    spheres[81].center.x = 142.0;

    // Right eye (index 82)
    spheres[82].center = Vec3::mapYZ(o, bodyV, bodyU, 69.0, 7.0);
    spheres[82].center.x = 142.0;

    // Hair (index 83)
    spheres[83].center = Vec3::ray(o, bodyV, 71.0);
    spheres[83].center.x = 152.0;
}

void Animation::updateAppendage(
    std::vector<SphereData>& spheres, int baseIndex,
    const Vec3& p, const Vec3& q, const Vec3& normalAxis,
    double A, double B, int countA, int countB) {

    Vec3 V = q - p;
    double D = V.length();
    double inverseD = 1.0 / D;
    V = V * inverseD;

    Vec3 W = normalAxis.normalized();
    Vec3 U = Vec3::cross(V, W);

    double A2 = A * A;
    double y = 0.5 * inverseD * (A2 - B * B + D * D);
    double square = A2 - y * y;
    if (square < 0) {
        square = 0; // Clamp to avoid sqrt of negative
    }
    double x = std::sqrt(square);

    // Joint position
    Vec3 j = Vec3::map(p, U, V, x, y);

    // Upper segment: countA+1 spheres from p to j
    Vec3 d = (j - p) * (1.0 / countA);
    for (int i = 0; i <= countA; i++) {
        spheres[baseIndex + i].center = Vec3::ray(p, d, (double)i);
    }

    // Lower segment: countB spheres from q toward j
    d = (j - q) * (1.0 / countB);
    for (int i = 0; i < countB; i++) {
        spheres[countA + 1 + baseIndex + i].center = Vec3::ray(q, d, (double)i);
    }
}
