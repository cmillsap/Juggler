#include "common.hlsli"

[shader("closesthit")]
void GroundClosestHit(inout RayPayload payload : SV_RayPayload,
                       in ProceduralAttributes attr : SV_IntersectionAttributes) {

    float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normal = attr.normal; // (0, 1, 0) for ground
    float3 rayDir = WorldRayDirection();

    // Ensure normal faces the ray
    if (dot(normal, rayDir) >= 0) {
        normal = -normal;
    }

    GPUMaterialData mat = g_Materials[attr.materialIndex];

    // Initialize RNG
    uint seed = pcg_hash(asuint(hitPoint.x) ^ pcg_hash(asuint(hitPoint.z) + frameCount));

    float3 color = float3(0, 0, 0);

    // --- Ambient with AO ---
    if (mat.ambientWeight > 0) {
        float ambientPercent = 1.0;

        if (mat.ambientOcclusionPercent > 0) {
            float3 aoDir = randomUnitVector(seed);
            if (dot(aoDir, normal) < 0)
                aoDir = -aoDir;

            if (isAOOccluded(hitPoint, aoDir, maxOcclusionDist)) {
                ambientPercent = 1.0 - mat.ambientOcclusionPercent;
            }
        }

        color += mat.ambientWeight * mat.diffuseColor * ambientColor * ambientPercent;
    }

    // --- Light direction with soft shadows ---
    float3 lightSamplePos = lightPos;
    if (lightRadius > 0) {
        float3 toHit = hitPoint - lightPos;
        float3 jitter = randomUnitVector(seed);
        if (dot(jitter, toHit) < 0)
            jitter = -jitter;
        lightSamplePos = lightPos + jitter * lightRadius;
    }

    float3 L = normalize(lightSamplePos - hitPoint);
    float NdotL = dot(L, normal);

    if (NdotL > 0) {
        float lightDist = length(lightSamplePos - hitPoint);
        bool illuminated = !isOccluded(hitPoint, L, lightDist);

        if (illuminated) {
            // Diffuse
            if (mat.diffuseWeight > 0) {
                color += mat.diffuseWeight * mat.diffuseColor * lightColor * NdotL;
            }

            // No specular for matte ground materials (specularWeight = 0)
        }
    }

    payload.color = color;
    payload.hasReflection = 0; // Ground is matte
}
