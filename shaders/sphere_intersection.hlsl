#include "common.hlsli"

[shader("intersection")]
void SphereIntersection() {
    uint primIndex = PrimitiveIndex();

    GPUSphereData sphere = g_Spheres[primIndex];

    float3 origin = ObjectRayOrigin();
    float3 dir = ObjectRayDirection();

    float3 oc = origin - sphere.center;
    float B = 2.0 * dot(dir, oc);
    float C = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = B * B - 4.0 * C;

    if (discriminant >= 0) {
        float sqrtDisc = sqrt(discriminant);
        float t1 = 0.5 * (-B - sqrtDisc);
        float t2 = 0.5 * (-B + sqrtDisc);

        float t = -1;
        if (t1 >= EPSILON && t1 <= RayTCurrent()) {
            t = t1;
        } else if (t2 >= EPSILON && t2 <= RayTCurrent()) {
            t = t2;
        }

        if (t > 0) {
            float3 hitPoint = origin + dir * t;
            float3 normal = normalize(hitPoint - sphere.center);

            ProceduralAttributes attr;
            attr.normal = normal;

            // Determine material index from sphere data
            // We need to look up the material for this sphere
            // Material index is stored based on sphere type
            // Sphere indices 0..2 = mirror (MAT_MIRROR=2)
            // Sphere indices 3..10 = torso (MAT_TORSO=3)
            // Sphere index 11 = head/skin (MAT_SKIN=4)
            // Sphere index 12 = neck/skin (MAT_SKIN=4)
            // Sphere indices 13..80 = limbs/skin (MAT_SKIN=4)
            // Sphere index 81 = left eye (MAT_EYE=5)
            // Sphere index 82 = right eye (MAT_EYE=5)
            // Sphere index 83 = hair (MAT_HAIR=6)

            uint matIdx;
            if (primIndex <= 2)
                matIdx = 2; // MIRROR
            else if (primIndex <= 10)
                matIdx = 3; // TORSO
            else if (primIndex <= 80)
                matIdx = 4; // SKIN
            else if (primIndex <= 82)
                matIdx = 5; // EYE
            else
                matIdx = 6; // HAIR

            attr.materialIndex = matIdx;

            ReportHit(t, 0, attr);
        }
    }
}
