#include "camera.h"
#include <cmath>

static constexpr double VIRTUAL_SCREEN_WIDTH = 100.0;
static constexpr double DISTANCE_TO_VIRTUAL_SCREEN = 50.0;

void Camera::init(int screenWidth, int screenHeight) {
    eye = { 2.0, 100.0, -2.0 };
    look = { 1000.0, 77.0, -1000.0 };
    distanceToScreen = DISTANCE_TO_VIRTUAL_SCREEN;
    virtualScreenRatio = VIRTUAL_SCREEN_WIDTH / screenWidth;
    halfWidth = screenWidth / 2.0;
    halfHeight = screenHeight / 2.0;

    // w = normalize(eye - look)
    w = Vec3::unitVector(eye, look);

    // Virtual screen center: c = eye + w * (-distance)
    virtualScreenCenter = Vec3::ray(eye, w, -distanceToScreen);

    // Build ONB from w
    Vec3::onb(u, v, w);
}

void Camera::updateOrbit(const Vec3& target, double angleRad, double radius, double height) {
    eye  = { target.x + radius * std::cos(angleRad), height, target.z + radius * std::sin(angleRad) };
    look = target;
    w    = Vec3::unitVector(eye, look);
    virtualScreenCenter = Vec3::ray(eye, w, -distanceToScreen);
    Vec3::onb(u, v, w);
}
