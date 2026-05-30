#include <gtest/gtest.h>
#include "animation.h"
#include "scene.h"
#include <cmath>

class AnimationTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene.init();
        anim = Animation();
    }

    Scene scene;
    Animation anim;
};

TEST_F(AnimationTest, BallPositionsAtT0) {
    anim.update(0.0, scene.spheres);

    // Ball 0 (low throw at T=0): z = X0 + L_VX * 0 = -182, y = Y0 + (L_VY - 0) * 0 = 88
    EXPECT_NEAR(scene.spheres[0].center.z, -182.0, 1e-6);
    EXPECT_NEAR(scene.spheres[0].center.y, 88.0, 1e-6);
    EXPECT_NEAR(scene.spheres[0].center.x, 110.0, 1e-6);

    // Ball 2 (high throw at T=0): z = X1 + H_VX * 0 = -108, y = Y0 = 88
    EXPECT_NEAR(scene.spheres[2].center.z, -108.0, 1e-6);
    EXPECT_NEAR(scene.spheres[2].center.y, 88.0, 1e-6);
}

TEST_F(AnimationTest, BallPositionsAtT15) {
    anim.update(15.0, scene.spheres);

    // Ball 0 (low throw at T=15): z = -182 + L_VX * 15
    double expectedZ = Animation::JUGGLE_X0 + Animation::JUGGLE_L_VX * 15.0;
    EXPECT_NEAR(scene.spheres[0].center.z, expectedZ, 1e-6);

    // y = Y0 + (L_VY - 0.5*G*T)*T
    double expectedY = Animation::JUGGLE_Y0 +
        (Animation::JUGGLE_L_VY - 0.5 * Animation::JUGGLE_G * 15.0) * 15.0;
    EXPECT_NEAR(scene.spheres[0].center.y, expectedY, 1e-6);
}

TEST_F(AnimationTest, BodyOscillationAtT0) {
    anim.update(0.0, scene.spheres);

    // At T=0: angle=0, cos(0)=1, oscillation=1.0
    // hips y = HIPS_MIN_Y + (HIPS_MAX_Y - HIPS_MIN_Y) * 1.0 = 85.0
    // First torso sphere (index 3) should be at hips position
    EXPECT_NEAR(scene.spheres[3].center.x, 151.0, 1e-6);
    EXPECT_NEAR(scene.spheres[3].center.y, 85.0, 0.1); // At hips
    EXPECT_NEAR(scene.spheres[3].center.z, -151.0, 1e-6);
}

TEST_F(AnimationTest, BodyOscillationAtT15) {
    anim.update(15.0, scene.spheres);

    // At T=15: angle = 2*PI/30 * 15 = PI, cos(PI) = -1, oscillation = 0.0
    // hips y = HIPS_MIN_Y + 0 = 81.0
    EXPECT_NEAR(scene.spheres[3].center.y, 81.0, 0.1);
}

TEST_F(AnimationTest, HeadFollowsBody) {
    anim.update(0.0, scene.spheres);

    // Head (index 11) should be above torso
    double headY = scene.spheres[11].center.y;
    double torsoTopY = scene.spheres[10].center.y;
    EXPECT_GT(headY, torsoTopY);
}

TEST_F(AnimationTest, IKJointPositionsReasonable) {
    anim.update(0.0, scene.spheres);

    // Left leg starts at indices 13..29
    // Foot (index 13) should be near ground level
    EXPECT_NEAR(scene.spheres[13].center.y, 2.5, 1e-6);

    // Right leg foot (index 30)
    EXPECT_NEAR(scene.spheres[30].center.y, 2.5, 1e-6);
}

TEST_F(AnimationTest, BallsHaveCorrectRadius) {
    // All 3 balls should have radius 14
    for (int i = 0; i < 3; i++) {
        EXPECT_DOUBLE_EQ(scene.spheres[i].radius, 14.0);
    }
}

TEST_F(AnimationTest, AnimationCycleConsistency) {
    // T=0 and T=30 should give same positions (30-frame cycle)
    anim.update(0.0, scene.spheres);
    Vec3 pos0 = scene.spheres[3].center;

    scene.init(); // Reset
    anim.update(30.0, scene.spheres);
    Vec3 pos30 = scene.spheres[3].center;

    EXPECT_NEAR(pos0.x, pos30.x, 1e-6);
    EXPECT_NEAR(pos0.y, pos30.y, 1e-6);
    EXPECT_NEAR(pos0.z, pos30.z, 1e-6);
}
