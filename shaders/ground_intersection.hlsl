#include "common.hlsli"

[shader("intersection")]
void GroundIntersection() {
    float3 origin = ObjectRayOrigin();
    float3 dir = ObjectRayDirection();

    // Ground plane at y = 0
    if (abs(dir.y) < 1e-8)
        return;

    float t = -origin.y / dir.y;

    if (t < EPSILON || t > RayTCurrent())
        return;

    float3 hitPoint = origin + dir * t;

    // Checkerboard material selection
    // Matches Java: ((floor(x/107) & 1) + (floor(z/107) & 1)) & 1
    int ix = (int)floor(hitPoint.x * invGroundSquareSize);
    int iz = (int)floor(hitPoint.z * invGroundSquareSize);

    uint matIdx = (((ix & 1) + (iz & 1)) & 1) == 0 ? MAT_GREEN_MATTE : MAT_YELLOW_MATTE;

    ProceduralAttributes attr;
    attr.normal = float3(0, 1, 0);
    attr.materialIndex = matIdx;

    ReportHit(t, 0, attr);
}
