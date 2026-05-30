#include "scene.h"
#include <cmath>

static constexpr double GAMMA = 2.2;

double Scene::gammaEncode(double value) {
    return std::pow(value, GAMMA);
}

void Scene::createColor(double color[3], int hexColor, double scale) {
    for (int i = 2; i >= 0; i--) {
        int intensity = hexColor & 0xFF;
        hexColor >>= 8;
        color[i] = std::pow(scale * intensity / 255.0, GAMMA);
    }
}

void Scene::createMaterials() {
    materials.resize(MAT_COUNT);

    // YELLOW_MATTE - ground
    materials[MAT_YELLOW_MATTE] = {
        0.13320851318429970246653555493722, 1.0, 1.5, 0.0, 0.0, 0.0,
        {1.0, 1.0, 0.0}, {0, 0, 0}, {0, 0, 0}
    };

    // GREEN_MATTE - ground
    materials[MAT_GREEN_MATTE] = {
        0.13320851318429970246653555493722, 1.0, 1.5, 0.0, 0.0, 0.0,
        {0.0, 1.0, 0.0}, {0, 0, 0}, {0, 0, 0}
    };

    // MIRROR (polished metal white)
    materials[MAT_MIRROR] = {
        0.0, 0.0, 0.0, 1.0, 1.0, 20.0,
        {1, 1, 1}, {1, 1, 1}, {1, 1, 1}
    };

    // TORSO - plastic from 0xE51715
    {
        double color[3];
        createColor(color, 0xE51715, 1.05);
        materials[MAT_TORSO] = {
            0.016988052089250049403595337516742, 1.0, 1.0, 1.0, 0.0, 10.0,
            {color[0], color[1], color[2]}, {1, 1, 1}, {0, 0, 0}
        };
    }

    // SKIN - plastic from 0xF2ADAB
    {
        double color[3];
        createColor(color, 0xF2ADAB, 1.05);
        materials[MAT_SKIN] = {
            0.016988052089250049403595337516742, 1.0, 1.0, 1.0, 0.0, 10.0,
            {color[0], color[1], color[2]}, {1, 1, 1}, {0, 0, 0}
        };
    }

    // EYE - plastic from 0x1E1B94
    {
        double color[3];
        createColor(color, 0x1E1B94, 1.4);
        materials[MAT_EYE] = {
            0.016988052089250049403595337516742, 1.0, 1.0, 1.0, 0.0, 10.0,
            {color[0], color[1], color[2]}, {1, 1, 1}, {0, 0, 0}
        };
    }

    // HAIR - plastic from 0x261117
    {
        double color[3];
        createColor(color, 0x261117, 1.4);
        materials[MAT_HAIR] = {
            0.016988052089250049403595337516742, 1.0, 1.0, 1.0, 0.0, 10.0,
            {color[0], color[1], color[2]}, {1, 1, 1}, {0, 0, 0}
        };
    }
}

void Scene::init() {
    createMaterials();
    spheres.resize(NUM_SPHERES);

    // Juggling balls (Java 2..4 -> 0..2)
    for (int i = 0; i < 3; i++) {
        spheres[i] = { {110.0, 0.0, 0.0}, 14.0, MAT_MIRROR };
    }

    // Torso (Java 5..12 -> 3..10)
    for (int i = 0; i < 8; i++) {
        double percent = i / 7.0;
        spheres[3 + i] = {
            {151.0, 85.0 + 32.0 * percent, -151.0},
            16.0 + 4.0 * percent,
            MAT_TORSO
        };
    }

    // Head (Java 13 -> 11)
    spheres[11] = { {151.0, 155.0, -151.0}, 14.0, MAT_SKIN };

    // Neck (Java 14 -> 12)
    spheres[12] = { {151.0, 140.0, -151.0}, 5.0, MAT_SKIN };

    // Legs and arms: 4 appendages with 17 spheres each
    // Left leg: Java 15..31 -> 13..29
    // Right leg: Java 32..48 -> 30..46
    // Left arm: Java 49..65 -> 47..63
    // Right arm: Java 66..82 -> 64..80
    for (int limb = 0; limb < 4; limb++) {
        int baseIdx = 13 + limb * 17;
        // Upper segment: 8 spheres with varying radius
        for (int i = 0; i < 8; i++) {
            spheres[baseIdx + i] = {
                {0, 0, 0},
                2.5 + 2.5 * i / 7.0,
                MAT_SKIN
            };
        }
        // Lower segment: 9 spheres with radius 5
        for (int i = 0; i < 9; i++) {
            spheres[baseIdx + 8 + i] = {
                {0, 0, 0},
                5.0,
                MAT_SKIN
            };
        }
    }

    // Left eye (Java 83 -> 81)
    spheres[81] = { {142.0, 154.0, -144.0}, 4.0, MAT_EYE };

    // Right eye (Java 84 -> 82)
    spheres[82] = { {142.0, 154.0, -144.0}, 4.0, MAT_EYE };

    // Hair (Java 85 -> 83)
    spheres[83] = { {152.0, 156.0, -151.0}, 14.0, MAT_HAIR };
}
