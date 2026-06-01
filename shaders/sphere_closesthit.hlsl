#include "common.hlsli"

[shader("closesthit")]
void SphereClosestHit(inout RayPayload payload : SV_RayPayload,
                       in ProceduralAttributes attr : SV_IntersectionAttributes) {

    float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 normal = attr.normal;
    float3 rayDir = WorldRayDirection();

    // Ensure normal faces the ray
    if (dot(normal, rayDir) >= 0) {
        normal = -normal;
    }

    // Offset origin along outward normal to prevent self-intersection in secondary rays
    float3 offsetOrigin = hitPoint + normal * SHADOW_BIAS;

    GPUMaterialData mat = g_Materials[attr.materialIndex];

    // Initialize RNG from hit position
    uint seed = pcg_hash(asuint(hitPoint.x) ^ pcg_hash(asuint(hitPoint.y) ^ pcg_hash(asuint(hitPoint.z) + frameCount + payload.sampleSeed)));

    float3 color = float3(0, 0, 0);

    // --- Ambient with AO ---
    if (mat.ambientWeight > 0) {
        float ambientPercent = 1.0;

        if (mat.ambientOcclusionPercent > 0) {
            float3 aoDir = randomUnitVector(seed);
            if (dot(aoDir, normal) < 0)
                aoDir = -aoDir;

            if (isAOOccluded(offsetOrigin, aoDir, maxOcclusionDist)) {
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
        bool illuminated = !isOccluded(offsetOrigin, L, lightDist);

        if (illuminated) {
            // Diffuse
            if (mat.diffuseWeight > 0) {
                color += mat.diffuseWeight * mat.diffuseColor * lightColor * NdotL;
            }

            // Specular (Blinn-Phong style from Java reference)
            if (mat.specularWeight > 0) {
                float3 R = 2.0 * NdotL * normal - L;
                float RdotMd = dot(R, -rayDir);
                if (RdotMd > 0) {
                    color += mat.specularWeight * pow(RdotMd, mat.shininess) *
                             lightColor * NdotL * mat.highlightColor;
                }
            }
        }
    }

    payload.color = color;

    // --- Reflection ---
    if (mat.reflectionWeight > 0) {
        payload.hasReflection = 1;
        payload.reflectionAttenuation = mat.reflectionWeight * mat.reflectionColor;
        payload.reflectionDir = reflect(rayDir, normal);
        payload.hitPoint = offsetOrigin;
    } else {
        payload.hasReflection = 0;
    }
}
