#pragma once

#include "math_utils.h"
#include <vector>
#include <cstdint>

struct SphereData {
    Vec3 center;
    double radius;
    int materialIndex;
};

struct MaterialData {
    double ambientWeight;
    double ambientOcclusionPercent;
    double diffuseWeight;
    double specularWeight;
    double reflectionWeight;
    double shininess;
    double diffuseColor[3];
    double highlightColor[3];
    double reflectionColor[3];
};

// Material indices
enum MaterialIndex : int {
    MAT_YELLOW_MATTE = 0,
    MAT_GREEN_MATTE = 1,
    MAT_MIRROR = 2,
    MAT_TORSO = 3,
    MAT_SKIN = 4,
    MAT_EYE = 5,
    MAT_HAIR = 6,
    MAT_COUNT = 7
};

class Scene {
public:
    static constexpr int NUM_SPHERES = 84;  // Excludes ground and sky

    std::vector<SphereData> spheres;
    std::vector<MaterialData> materials;

    void init();

    // Sphere index mapping (Java scene index -> our sphere array index):
    // Java 2..4 -> 0..2 (juggling balls)
    // Java 5..12 -> 3..10 (torso)
    // Java 13 -> 11 (head)
    // Java 14 -> 12 (neck)
    // Java 15..31 -> 13..29 (left leg)
    // Java 32..48 -> 30..46 (right leg)
    // Java 49..65 -> 47..63 (left arm)
    // Java 66..82 -> 64..80 (right arm)
    // Java 83 -> 81 (left eye)
    // Java 84 -> 82 (right eye)
    // Java 85 -> 83 (hair)

private:
    static double gammaEncode(double value);
    static void createColor(double color[3], int hexColor, double scale);
    void createMaterials();
};
