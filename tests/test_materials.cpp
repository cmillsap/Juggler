#include <gtest/gtest.h>
#include "scene.h"
#include <cmath>

class MaterialsTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene.init();
    }
    Scene scene;
};

TEST_F(MaterialsTest, TorsoColorGammaEncoded) {
    // TORSO: createColor(0xE51715, 1.05)
    // R = 0xE5 = 229, pow(1.05 * 229/255, 2.2)
    const auto& mat = scene.materials[MAT_TORSO];
    double expectedR = std::pow(1.05 * 0xE5 / 255.0, 2.2);
    double expectedG = std::pow(1.05 * 0x17 / 255.0, 2.2);
    double expectedB = std::pow(1.05 * 0x15 / 255.0, 2.2);

    EXPECT_NEAR(mat.diffuseColor[0], expectedR, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[1], expectedG, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[2], expectedB, 1e-6);
}

TEST_F(MaterialsTest, SkinColorGammaEncoded) {
    // SKIN: createColor(0xF2ADAB, 1.05)
    const auto& mat = scene.materials[MAT_SKIN];
    double expectedR = std::pow(1.05 * 0xF2 / 255.0, 2.2);
    double expectedG = std::pow(1.05 * 0xAD / 255.0, 2.2);
    double expectedB = std::pow(1.05 * 0xAB / 255.0, 2.2);

    EXPECT_NEAR(mat.diffuseColor[0], expectedR, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[1], expectedG, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[2], expectedB, 1e-6);
}

TEST_F(MaterialsTest, EyeColorGammaEncoded) {
    // EYE: createColor(0x1E1B94, 1.4)
    const auto& mat = scene.materials[MAT_EYE];
    double expectedR = std::pow(1.4 * 0x1E / 255.0, 2.2);
    double expectedG = std::pow(1.4 * 0x1B / 255.0, 2.2);
    double expectedB = std::pow(1.4 * 0x94 / 255.0, 2.2);

    EXPECT_NEAR(mat.diffuseColor[0], expectedR, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[1], expectedG, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[2], expectedB, 1e-6);
}

TEST_F(MaterialsTest, HairColorGammaEncoded) {
    // HAIR: createColor(0x261117, 1.4)
    const auto& mat = scene.materials[MAT_HAIR];
    double expectedR = std::pow(1.4 * 0x26 / 255.0, 2.2);
    double expectedG = std::pow(1.4 * 0x11 / 255.0, 2.2);
    double expectedB = std::pow(1.4 * 0x17 / 255.0, 2.2);

    EXPECT_NEAR(mat.diffuseColor[0], expectedR, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[1], expectedG, 1e-6);
    EXPECT_NEAR(mat.diffuseColor[2], expectedB, 1e-6);
}

TEST_F(MaterialsTest, PlasticProperties) {
    // All plastic materials share the same structure
    const auto& torso = scene.materials[MAT_TORSO];
    EXPECT_NEAR(torso.ambientWeight, 0.016988052089250049, 1e-12);
    EXPECT_DOUBLE_EQ(torso.ambientOcclusionPercent, 1.0);
    EXPECT_DOUBLE_EQ(torso.diffuseWeight, 1.0);
    EXPECT_DOUBLE_EQ(torso.specularWeight, 1.0);
    EXPECT_DOUBLE_EQ(torso.reflectionWeight, 0.0);
    EXPECT_DOUBLE_EQ(torso.shininess, 10.0);
    // Highlight color = white for plastics
    EXPECT_DOUBLE_EQ(torso.highlightColor[0], 1.0);
    EXPECT_DOUBLE_EQ(torso.highlightColor[1], 1.0);
    EXPECT_DOUBLE_EQ(torso.highlightColor[2], 1.0);
}

TEST_F(MaterialsTest, MatteProperties) {
    const auto& yellow = scene.materials[MAT_YELLOW_MATTE];
    EXPECT_NEAR(yellow.ambientWeight, 0.13320851318429970, 1e-12);
    EXPECT_DOUBLE_EQ(yellow.diffuseWeight, 1.5);
    EXPECT_DOUBLE_EQ(yellow.specularWeight, 0.0);
    EXPECT_DOUBLE_EQ(yellow.reflectionWeight, 0.15);
}

TEST_F(MaterialsTest, MirrorProperties) {
    const auto& mirror = scene.materials[MAT_MIRROR];
    EXPECT_DOUBLE_EQ(mirror.ambientWeight, 0.0);
    EXPECT_DOUBLE_EQ(mirror.diffuseWeight, 0.0);
    EXPECT_DOUBLE_EQ(mirror.specularWeight, 1.0);
    EXPECT_DOUBLE_EQ(mirror.reflectionWeight, 1.0);
    EXPECT_DOUBLE_EQ(mirror.shininess, 20.0);
    EXPECT_DOUBLE_EQ(mirror.reflectionColor[0], 1.0);
    EXPECT_DOUBLE_EQ(mirror.reflectionColor[1], 1.0);
    EXPECT_DOUBLE_EQ(mirror.reflectionColor[2], 1.0);
}
