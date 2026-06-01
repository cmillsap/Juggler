#include "common.hlsli"

[shader("raygeneration")]
void RayGen() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    float3 totalColor = float3(0, 0, 0);

    for (uint s = 0; s < SAMPLES_PER_PIXEL; s++) {
        // Vary seed per sample so each gets independent jitter, AO, and shadow directions
        uint seed = pcg_hash(launchIndex.x + launchIndex.y * launchDim.x +
                             (frameCount * SAMPLES_PER_PIXEL + s) * launchDim.x * launchDim.y);

        // Jittered pixel position for anti-aliasing
        float jitterX = rand(seed);
        float jitterY = rand(seed);

        float px = (float)launchIndex.x + jitterX;
        float py = (float)launchIndex.y + jitterY;

        // Compute ray from camera (matching Java reference)
        float a = virtualScreenRatio * (px - halfWidth);
        float b = virtualScreenRatio * (halfHeight - py);

        float3 screenPoint = virtualScreenCenter + a * cameraU + b * cameraV;
        float3 rayDir = normalize(screenPoint - cameraPos);
        float3 rayOrigin = cameraPos;

        // Iterative reflection loop
        float3 pixelColor = float3(0, 0, 0);
        float3 throughput = float3(1, 1, 1);

        for (int bounce = 0; bounce < MAX_DEPTH; bounce++) {
            RayDesc ray;
            ray.Origin = rayOrigin;
            ray.Direction = rayDir;
            ray.TMin = EPSILON;
            ray.TMax = 1e20;

            RayPayload payload;
            payload.color = float3(0, 0, 0);
            payload.reflectionAttenuation = float3(0, 0, 0);
            payload.hitPoint = float3(0, 0, 0);
            payload.reflectionDir = float3(0, 0, 0);
            payload.hasReflection = 0;
            payload.sampleSeed = s * 1664525u;

            TraceRay(g_TLAS,
                RAY_FLAG_NONE,
                0xFF,
                0,      // RayContributionToHitGroupIndex
                1,      // MultiplierForGeometryContributionToHitGroupIndex
                0,      // MissShaderIndex
                ray,
                payload);

            pixelColor += throughput * payload.color;

            if (payload.hasReflection == 0)
                break;

            // Update for next bounce
            throughput *= payload.reflectionAttenuation;

            // Early out if contribution too small
            if (max(max(throughput.x, throughput.y), throughput.z) < MIN_COLOR_INTENSITY)
                break;

            rayOrigin = payload.hitPoint;
            rayDir = payload.reflectionDir;
        }

        totalColor += pixelColor;
    }

    float3 frameColor = totalColor / float(SAMPLES_PER_PIXEL);

    // Accumulate
    float4 prevAccum = g_Accum[launchIndex];
    float4 newAccum;
    if (accumulatedFrames <= 1) {
        newAccum = float4(frameColor, 1.0);
    } else {
        newAccum = prevAccum + float4(frameColor, 1.0);
    }
    g_Accum[launchIndex] = newAccum;

    // Average and gamma correct
    float3 averaged = newAccum.rgb / newAccum.w;
    float3 gammaCorrected = pow(saturate(averaged), INV_GAMMA);

    g_Output[launchIndex] = float4(gammaCorrected, 1.0);
}
