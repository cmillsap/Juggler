#pragma once

#include <cstdint>

// CPU-side structures matching GPU layout (16-byte aligned rows)

struct Float3 { float x, y, z; };

struct PerFrameConstants {
    Float3   cameraPos;
    uint32_t frameCount;

    Float3 cameraU;
    float  virtualScreenRatio;

    Float3 cameraV;
    float  distanceToScreen;

    Float3   virtualScreenCenter;
    uint32_t numSpheres;

    Float3 lightPos;
    float  lightRadius;

    Float3 lightColor;
    float  groundSquareSize;

    Float3 ambientColor;
    float  invGroundSquareSize;

    Float3 skyMinColor;
    float  maxOcclusionDist;

    Float3   skyMaxColor;
    uint32_t accumulatedFrames;

    uint32_t screenSizeX;
    uint32_t screenSizeY;
    float    halfWidth;
    float    halfHeight;
};

struct GPUSphereData {
    Float3 center;
    float  radius;
};

struct GPUMaterialData {
    Float3 diffuseColor;
    float  ambientWeight;

    Float3 highlightColor;
    float  ambientOcclusionPercent;

    Float3 reflectionColor;
    float  diffuseWeight;

    float specularWeight;
    float reflectionWeight;
    float shininess;
    float padding;
};
