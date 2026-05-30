#include <gtest/gtest.h>
#include "scene.h"

class SceneTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene.init();
    }
    Scene scene;
};

TEST_F(SceneTest, CorrectSphereCount) {
    EXPECT_EQ(scene.spheres.size(), (size_t)Scene::NUM_SPHERES);
    EXPECT_EQ(scene.spheres.size(), 84u);
}

TEST_F(SceneTest, CorrectMaterialCount) {
    EXPECT_EQ(scene.materials.size(), (size_t)MAT_COUNT);
    EXPECT_EQ(scene.materials.size(), 7u);
}

TEST_F(SceneTest, JugglingBallsMirrorMaterial) {
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(scene.spheres[i].materialIndex, MAT_MIRROR);
        EXPECT_DOUBLE_EQ(scene.spheres[i].radius, 14.0);
    }
}

TEST_F(SceneTest, TorsoMaterial) {
    for (int i = 3; i <= 10; i++) {
        EXPECT_EQ(scene.spheres[i].materialIndex, MAT_TORSO);
    }
}

TEST_F(SceneTest, TorsoRadiusGradient) {
    // Torso radii should increase from 16 to 20
    EXPECT_NEAR(scene.spheres[3].radius, 16.0, 1e-6);
    EXPECT_NEAR(scene.spheres[10].radius, 20.0, 1e-6);
}

TEST_F(SceneTest, HeadAndNeckSkin) {
    EXPECT_EQ(scene.spheres[11].materialIndex, MAT_SKIN); // head
    EXPECT_EQ(scene.spheres[12].materialIndex, MAT_SKIN); // neck
    EXPECT_DOUBLE_EQ(scene.spheres[11].radius, 14.0);
    EXPECT_DOUBLE_EQ(scene.spheres[12].radius, 5.0);
}

TEST_F(SceneTest, LimbsSkinMaterial) {
    // All 4 limbs (indices 13..80) should be skin
    for (int i = 13; i <= 80; i++) {
        EXPECT_EQ(scene.spheres[i].materialIndex, MAT_SKIN)
            << "Sphere " << i << " should be SKIN material";
    }
}

TEST_F(SceneTest, EyesMaterial) {
    EXPECT_EQ(scene.spheres[81].materialIndex, MAT_EYE);
    EXPECT_EQ(scene.spheres[82].materialIndex, MAT_EYE);
    EXPECT_DOUBLE_EQ(scene.spheres[81].radius, 4.0);
}

TEST_F(SceneTest, HairMaterial) {
    EXPECT_EQ(scene.spheres[83].materialIndex, MAT_HAIR);
    EXPECT_DOUBLE_EQ(scene.spheres[83].radius, 14.0);
}

TEST_F(SceneTest, MirrorMaterialIsReflective) {
    const auto& mat = scene.materials[MAT_MIRROR];
    EXPECT_DOUBLE_EQ(mat.reflectionWeight, 1.0);
    EXPECT_DOUBLE_EQ(mat.diffuseWeight, 0.0);
    EXPECT_DOUBLE_EQ(mat.ambientWeight, 0.0);
}

TEST_F(SceneTest, MatteIsNotReflective) {
    const auto& yellow = scene.materials[MAT_YELLOW_MATTE];
    EXPECT_DOUBLE_EQ(yellow.reflectionWeight, 0.0);
    EXPECT_DOUBLE_EQ(yellow.specularWeight, 0.0);
    EXPECT_GT(yellow.diffuseWeight, 0.0);
}
