#include <gtest/gtest.h>
#include "camera.h"
#include <cmath>

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override {
        cam.init(1920, 1080);
    }
    Camera cam;
};

TEST_F(CameraTest, EyePosition) {
    EXPECT_DOUBLE_EQ(cam.eye.x, 2.0);
    EXPECT_DOUBLE_EQ(cam.eye.y, 100.0);
    EXPECT_DOUBLE_EQ(cam.eye.z, -2.0);
}

TEST_F(CameraTest, LookPosition) {
    EXPECT_DOUBLE_EQ(cam.look.x, 1000.0);
    EXPECT_DOUBLE_EQ(cam.look.y, 77.0);
    EXPECT_DOUBLE_EQ(cam.look.z, -1000.0);
}

TEST_F(CameraTest, WIsNormalized) {
    EXPECT_NEAR(cam.w.length(), 1.0, 1e-9);
}

TEST_F(CameraTest, WDirection) {
    // w = normalize(eye - look) = normalize(-998, 23, 998)
    Vec3 expected = (cam.eye - cam.look).normalized();
    EXPECT_NEAR(cam.w.x, expected.x, 1e-9);
    EXPECT_NEAR(cam.w.y, expected.y, 1e-9);
    EXPECT_NEAR(cam.w.z, expected.z, 1e-9);
}

TEST_F(CameraTest, BasisOrthogonal) {
    EXPECT_NEAR(cam.u.dot(cam.v), 0, 1e-9);
    EXPECT_NEAR(cam.u.dot(cam.w), 0, 1e-9);
    EXPECT_NEAR(cam.v.dot(cam.w), 0, 1e-9);
}

TEST_F(CameraTest, VirtualScreenCenter) {
    // c = eye + w * (-distance)
    Vec3 expected = Vec3::ray(cam.eye, cam.w, -50.0);
    EXPECT_NEAR(cam.virtualScreenCenter.x, expected.x, 1e-6);
    EXPECT_NEAR(cam.virtualScreenCenter.y, expected.y, 1e-6);
    EXPECT_NEAR(cam.virtualScreenCenter.z, expected.z, 1e-6);
}

TEST_F(CameraTest, VirtualScreenRatio) {
    EXPECT_NEAR(cam.virtualScreenRatio, 100.0 / 1920.0, 1e-9);
}

TEST_F(CameraTest, CenterPixelRay) {
    // Center pixel: x = 960, y = 540
    // a = ratio * (960 - 960) = 0
    // b = ratio * (540 - 540) = 0
    // p = c + 0*u + 0*v = c
    // d = normalize(c - eye) = normalize(w * (-50)) = -w
    double a = cam.virtualScreenRatio * (960.0 - cam.halfWidth);
    double b = cam.virtualScreenRatio * (cam.halfHeight - 540.0);
    EXPECT_NEAR(a, 0, 1e-9);
    EXPECT_NEAR(b, 0, 1e-9);

    Vec3 p = cam.virtualScreenCenter;
    Vec3 d = Vec3::unitVector(p, cam.eye);
    // d should be -w (opposite direction to w)
    EXPECT_NEAR(d.x, -cam.w.x, 1e-9);
    EXPECT_NEAR(d.y, -cam.w.y, 1e-9);
    EXPECT_NEAR(d.z, -cam.w.z, 1e-9);
}
