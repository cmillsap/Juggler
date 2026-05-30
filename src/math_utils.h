#pragma once

#include <cmath>
#include <algorithm>

struct Vec3 {
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return { x + b.x, y + b.y, z + b.z }; }
    Vec3 operator-(const Vec3& b) const { return { x - b.x, y - b.y, z - b.z }; }
    Vec3 operator*(double s) const { return { x * s, y * s, z * s }; }
    Vec3 operator/(double s) const { double inv = 1.0 / s; return { x * inv, y * inv, z * inv }; }
    Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    Vec3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }

    Vec3 operator-() const { return { -x, -y, -z }; }

    double dot(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
    double length2() const { return x * x + y * y + z * z; }
    double length() const { return std::sqrt(length2()); }

    Vec3 normalized() const {
        double inv = 1.0 / length();
        return { x * inv, y * inv, z * inv };
    }

    void normalize() {
        double inv = 1.0 / length();
        x *= inv; y *= inv; z *= inv;
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static double distance(const Vec3& a, const Vec3& b) {
        return (a - b).length();
    }

    // Construct unit vector from head to tail: (head - tail) / |head - tail|
    static Vec3 unitVector(const Vec3& head, const Vec3& tail) {
        return (head - tail).normalized();
    }

    // ONB construction from w vector (matching Java Vec.onb)
    static void onb(Vec3& u, Vec3& v, const Vec3& w) {
        const double TINY = 1e-9;
        bool isUp = (std::abs(w.x) <= TINY && std::abs(w.y - 1.0) <= TINY && std::abs(w.z) <= TINY);
        bool isDown = (std::abs(w.x) <= TINY && std::abs(w.y + 1.0) <= TINY && std::abs(w.z) <= TINY);

        if (isUp) {
            u = { 1, 0, 0 };
            v = { 0, 0, -1 };
        } else if (isDown) {
            u = { 1, 0, 0 };
            v = { 0, 0, 1 };
        } else {
            u = Vec3{ w.z, 0, -w.x }.normalized();
            v = Vec3::cross(w, u);
        }
    }

    // Map: result = o + a*u + b*v
    static Vec3 map(const Vec3& o, const Vec3& u, const Vec3& v, double a, double b) {
        return { o.x + a * u.x + b * v.x,
                 o.y + a * u.y + b * v.y,
                 o.z + a * u.z + b * v.z };
    }

    // MapYZ: result = o + y*v + z*w
    static Vec3 mapYZ(const Vec3& o, const Vec3& v, const Vec3& w, double y, double z) {
        return { o.x + y * v.x + z * w.x,
                 o.y + y * v.y + z * w.y,
                 o.z + y * v.z + z * w.z };
    }

    // Ray: result = origin + dir * t
    static Vec3 ray(const Vec3& origin, const Vec3& dir, double t) {
        return { origin.x + dir.x * t, origin.y + dir.y * t, origin.z + dir.z * t };
    }
};

inline Vec3 operator*(double s, const Vec3& v) { return v * s; }
