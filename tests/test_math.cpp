#include <gtest/gtest.h>
#include "math_utils.h"
#include <cmath>

TEST(Vec3Test, Addition) {
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);
    Vec3 c = a + b;
    EXPECT_DOUBLE_EQ(c.x, 5);
    EXPECT_DOUBLE_EQ(c.y, 7);
    EXPECT_DOUBLE_EQ(c.z, 9);
}

TEST(Vec3Test, Subtraction) {
    Vec3 a(4, 5, 6);
    Vec3 b(1, 2, 3);
    Vec3 c = a - b;
    EXPECT_DOUBLE_EQ(c.x, 3);
    EXPECT_DOUBLE_EQ(c.y, 3);
    EXPECT_DOUBLE_EQ(c.z, 3);
}

TEST(Vec3Test, Scale) {
    Vec3 a(1, 2, 3);
    Vec3 b = a * 2.0;
    EXPECT_DOUBLE_EQ(b.x, 2);
    EXPECT_DOUBLE_EQ(b.y, 4);
    EXPECT_DOUBLE_EQ(b.z, 6);
}

TEST(Vec3Test, Normalize) {
    Vec3 a(3, 0, 4);
    Vec3 n = a.normalized();
    EXPECT_NEAR(n.x, 0.6, 1e-9);
    EXPECT_NEAR(n.y, 0.0, 1e-9);
    EXPECT_NEAR(n.z, 0.8, 1e-9);
    EXPECT_NEAR(n.length(), 1.0, 1e-9);
}

TEST(Vec3Test, DotProduct) {
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);
    EXPECT_DOUBLE_EQ(a.dot(b), 0);

    Vec3 c(1, 2, 3);
    Vec3 d(4, 5, 6);
    EXPECT_DOUBLE_EQ(c.dot(d), 32);
}

TEST(Vec3Test, CrossProduct) {
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);
    Vec3 c = Vec3::cross(a, b);
    EXPECT_DOUBLE_EQ(c.x, 0);
    EXPECT_DOUBLE_EQ(c.y, 0);
    EXPECT_DOUBLE_EQ(c.z, 1);
}

TEST(Vec3Test, Distance) {
    Vec3 a(0, 0, 0);
    Vec3 b(3, 4, 0);
    EXPECT_NEAR(Vec3::distance(a, b), 5.0, 1e-9);
}

TEST(Vec3Test, ONB_General) {
    Vec3 w(0, 0, 1);
    Vec3 u, v;
    Vec3::onb(u, v, w);
    // u should be perpendicular to w
    EXPECT_NEAR(u.dot(w), 0, 1e-9);
    // v should be perpendicular to both
    EXPECT_NEAR(v.dot(w), 0, 1e-9);
    EXPECT_NEAR(v.dot(u), 0, 1e-9);
}

TEST(Vec3Test, ONB_Up) {
    Vec3 w(0, 1, 0);
    Vec3 u, v;
    Vec3::onb(u, v, w);
    EXPECT_NEAR(u.x, 1, 1e-9);
    EXPECT_NEAR(u.y, 0, 1e-9);
    EXPECT_NEAR(u.z, 0, 1e-9);
    EXPECT_NEAR(v.x, 0, 1e-9);
    EXPECT_NEAR(v.y, 0, 1e-9);
    EXPECT_NEAR(v.z, -1, 1e-9);
}

TEST(Vec3Test, ONB_Down) {
    Vec3 w(0, -1, 0);
    Vec3 u, v;
    Vec3::onb(u, v, w);
    EXPECT_NEAR(u.x, 1, 1e-9);
    EXPECT_NEAR(u.y, 0, 1e-9);
    EXPECT_NEAR(u.z, 0, 1e-9);
    EXPECT_NEAR(v.x, 0, 1e-9);
    EXPECT_NEAR(v.y, 0, 1e-9);
    EXPECT_NEAR(v.z, 1, 1e-9);
}

TEST(Vec3Test, Map) {
    Vec3 o(1, 2, 3);
    Vec3 u(1, 0, 0);
    Vec3 v(0, 1, 0);
    Vec3 r = Vec3::map(o, u, v, 5.0, 10.0);
    EXPECT_DOUBLE_EQ(r.x, 6);
    EXPECT_DOUBLE_EQ(r.y, 12);
    EXPECT_DOUBLE_EQ(r.z, 3);
}

TEST(Vec3Test, Ray) {
    Vec3 o(0, 0, 0);
    Vec3 d(1, 0, 0);
    Vec3 r = Vec3::ray(o, d, 5.0);
    EXPECT_DOUBLE_EQ(r.x, 5);
    EXPECT_DOUBLE_EQ(r.y, 0);
    EXPECT_DOUBLE_EQ(r.z, 0);
}
