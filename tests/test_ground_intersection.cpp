#include <gtest/gtest.h>
#include "math_utils.h"
#include <cmath>

static const double EPSILON = 1e-4;
static const double GROUND_SQUARE_SIZE = 107.0;
static const double INV_GROUND_SQUARE_SIZE = 1.0 / 107.0;

struct GroundIntersectionResult {
    bool hit;
    double t;
    Vec3 hitPoint;
    int materialIndex; // 0 = yellow, 1 = green
};

GroundIntersectionResult intersectGround(
    const Vec3& origin, const Vec3& dir, double maxT = 1e20) {

    GroundIntersectionResult result = {};

    if (std::abs(dir.y) < 1e-8)
        return result;

    double t = -origin.y / dir.y;
    if (t < EPSILON || t > maxT)
        return result;

    result.hit = true;
    result.t = t;
    result.hitPoint = Vec3::ray(origin, dir, t);

    // Checkerboard
    long a = (long)std::floor(result.hitPoint.x * INV_GROUND_SQUARE_SIZE) & 1;
    long b = (long)std::floor(result.hitPoint.z * INV_GROUND_SQUARE_SIZE) & 1;
    result.materialIndex = ((a + b) & 1) == 0 ? 1 : 0; // 0=yellow, 1=green

    return result;
}

TEST(GroundIntersection, HitFromAbove) {
    Vec3 origin(0, 10, 0);
    Vec3 dir(0, -1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.t, 10.0, 1e-6);
    EXPECT_NEAR(result.hitPoint.y, 0.0, 1e-6);
}

TEST(GroundIntersection, MissParallel) {
    Vec3 origin(0, 10, 0);
    Vec3 dir(1, 0, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_FALSE(result.hit);
}

TEST(GroundIntersection, MissUpward) {
    Vec3 origin(0, 10, 0);
    Vec3 dir(0, 1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_FALSE(result.hit);
}

TEST(GroundIntersection, AngledHit) {
    Vec3 origin(0, 100, 0);
    Vec3 dir = Vec3(1, -1, 0).normalized();

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.hitPoint.y, 0.0, 1e-6);
    EXPECT_NEAR(result.hitPoint.x, 100.0, 1e-4);
}

TEST(GroundIntersection, CheckerboardOrigin) {
    // Point at (0, 10, 0) looking down -> hits (0, 0, 0)
    // floor(0/107) = 0, floor(0/107) = 0 -> (0 & 1) ^ (0 & 1) = 0
    Vec3 origin(0, 10, 0);
    Vec3 dir(0, -1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.materialIndex, 1); // green at origin
}

TEST(GroundIntersection, CheckerboardOffset) {
    // Point above (107.5, 0, 0) — center of the adjacent square to avoid the exact boundary
    // floor(107.5/107) = 1, floor(0/107) = 0 -> ((1+0)&1) != 0 -> yellow
    Vec3 origin(107.5, 10, 0);
    Vec3 dir(0, -1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.materialIndex, 0); // yellow
}

TEST(GroundIntersection, CheckerboardDiagonal) {
    // Point at (107, 10, 107)
    // floor(107/107) = 1, floor(107/107) = 1 -> (1 & 1) ^ (1 & 1) = 0
    Vec3 origin(107, 10, 107);
    Vec3 dir(0, -1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.materialIndex, 1); // green
}

TEST(GroundIntersection, CheckerboardNegative) {
    // Point at (-1, 10, 0)
    // floor(-1/107) = floor(-0.00935) = -1, floor(0) = 0
    // (-1 & 1) ^ (0 & 1) = 1 ^ 0 = 1
    Vec3 origin(-1, 10, 0);
    Vec3 dir(0, -1, 0);

    auto result = intersectGround(origin, dir);
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.materialIndex, 0); // yellow
}
