#include "common.hlsli"

[shader("miss")]
void Miss(inout RayPayload payload : SV_RayPayload) {
    // Sky gradient based on ray direction y component
    // Matches Java SkyMaterial: interpolate between minColor and maxColor
    float3 dir = WorldRayDirection();
    float t = saturate(dir.y);

    float3 skyColor = lerp(skyMinColor, skyMaxColor, t);

    // Sky only contributes ambient (ambientWeight=1 in Java)
    payload.color = skyColor;
    payload.hasReflection = 0;
}
