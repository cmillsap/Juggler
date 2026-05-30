#include <gtest/gtest.h>
#include "math_utils.h"
#include <cmath>

// CPU-side sphere intersection matching the GPU shader logic
struct SphereIntersectionResult {
    bool hit;
    double t;
    Vec3 normal;
};

static const double EPSILON = 1e-4;

SphereIntersectionResult intersectSphere(
    const Vec3& origin, const Vec3& dir,
    const Vec3& center, double radius, double maxT = 1e20) {

    SphereIntersectionResult result = {};
    Vec3 oc = origin - center;
    double B = 2.0 * dir.dot(oc);
    double C = oc.length2() - radius * radius;
    double disc = B * B - 4.0 * C;

    if (disc >= 0) {
        double sqrtDisc = std::sqrt(disc);
        double t1 = 0.5 * (-B - sqrtDisc);
        double t2 = 0.5 * (-B + sqrtDisc);

        if (t1 >= EPSILON && t1 <= maxT) {
            result.hit = true;
            result.t = t1;
        } else if (t2 >= EPSILON && t2 <= maxT) {
            result.hit = true;
            result.t = t2;
        }

        if (result.hit) {
            Vec3 hitPoint = Vec3::ray(origin, dir, result.t);
            result.normal = (hitPoint - center).normalized();
        }
    }
    return result;
}

TEST(SphereIntersection, DirectHit) {
    Vec3 origin(0, 0, -10);
    Vec3 dir(0, 0, 1);
    Vec3 center(0, 0, 0);
    double radius = 1.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.t, 9.0, 1e-6);
    EXPECT_NEAR(result.normal.z, -1.0, 1e-6);
}

TEST(SphereIntersection, Miss) {
    Vec3 origin(0, 5, -10);
    Vec3 dir(0, 0, 1);
    Vec3 center(0, 0, 0);
    double radius = 1.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_FALSE(result.hit);
}

TEST(SphereIntersection, InsideSphere) {
    Vec3 origin(0, 0, 0);
    Vec3 dir(0, 0, 1);
    Vec3 center(0, 0, 0);
    double radius = 5.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.t, 5.0, 1e-6);
}

TEST(SphereIntersection, TangentHit) {
    Vec3 origin(0, 1, -10);
    Vec3 dir(0, 0, 1);
    Vec3 center(0, 0, 0);
    double radius = 1.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.t, 10.0, 1e-4);
}

TEST(SphereIntersection, BehindRay) {
    Vec3 origin(0, 0, 10);
    Vec3 dir(0, 0, 1);
    Vec3 center(0, 0, 0);
    double radius = 1.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_FALSE(result.hit);
}

TEST(SphereIntersection, NormalDirection) {
    Vec3 origin(10, 0, 0);
    Vec3 dir(-1, 0, 0);
    Vec3 center(0, 0, 0);
    double radius = 1.0;

    auto result = intersectSphere(origin, dir, center, radius);
    EXPECT_TRUE(result.hit);
    EXPECT_NEAR(result.normal.x, 1.0, 1e-6);
    EXPECT_NEAR(result.normal.y, 0.0, 1e-6);
    EXPECT_NEAR(result.normal.z, 0.0, 1e-6);
}
