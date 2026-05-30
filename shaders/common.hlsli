// Common bindings, structures, and utilities for all shaders

// ------ Resource Bindings (Global Root Signature) ------
RaytracingAccelerationStructure g_TLAS   : register(t0);
RWTexture2D<float4>             g_Output : register(u0);
RWTexture2D<float4>             g_Accum  : register(u1);

struct GPUSphereData {
    float3 center;
    float  radius;
};

struct GPUMaterialData {
    float3 diffuseColor;
    float  ambientWeight;
    float3 highlightColor;
    float  ambientOcclusionPercent;
    float3 reflectionColor;
    float  diffuseWeight;
    float  specularWeight;
    float  reflectionWeight;
    float  shininess;
    float  padding;
};

StructuredBuffer<GPUSphereData>  g_Spheres   : register(t1);
StructuredBuffer<GPUMaterialData> g_Materials : register(t2);

cbuffer PerFrameConstants : register(b0) {
    float3 cameraPos;
    uint   frameCount;

    float3 cameraU;
    float  virtualScreenRatio;

    float3 cameraV;
    float  distanceToScreen;

    float3 virtualScreenCenter;
    uint   numSpheres;

    float3 lightPos;
    float  lightRadius;

    float3 lightColor;
    float  groundSquareSize;

    float3 ambientColor;
    float  invGroundSquareSize;

    float3 skyMinColor;
    float  maxOcclusionDist;

    float3 skyMaxColor;
    uint   accumulatedFrames;

    uint   screenSizeX;
    uint   screenSizeY;
    float  halfWidth;
    float  halfHeight;
};

// ------ Ray Payload ------
struct RayPayload {
    float3 color;               // Direct illumination color
    float3 reflectionAttenuation; // reflection weight * reflection color
    float3 hitPoint;            // Origin for reflection ray
    float3 reflectionDir;      // Reflection direction
    uint   hasReflection;      // Continue tracing? (0 = no, 1 = yes)
};

// ------ Procedural Intersection Attributes ------
struct ProceduralAttributes {
    float3 normal;
    uint   materialIndex;
};

// ------ PCG Random Number Generator ------
uint pcg_hash(uint input) {
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand(inout uint seed) {
    seed = pcg_hash(seed);
    return float(seed) / 4294967295.0;
}

float3 randomUnitVector(inout uint seed) {
    float3 v;
    float len2;
    do {
        v = float3(rand(seed) * 2.0 - 1.0, rand(seed) * 2.0 - 1.0, rand(seed) * 2.0 - 1.0);
        len2 = dot(v, v);
    } while (len2 > 1.0 || len2 < 1e-6);
    return normalize(v);
}

// ------ Constants ------
static const float EPSILON = 1e-4;
static const float SHADOW_BIAS = 0.05f;
static const float PI = 3.14159265358979323846;
static const int MAX_DEPTH = 10;
static const float MIN_COLOR_INTENSITY = 1.0 / 256.0;
static const float GAMMA = 2.2;
static const float INV_GAMMA = 1.0 / 2.2;

// Material indices
static const uint MAT_YELLOW_MATTE = 0;
static const uint MAT_GREEN_MATTE = 1;

// ------ Inline Ray Query Helpers ------

// Test if a point is in shadow (any hit between origin and light)
bool isOccluded(float3 origin, float3 direction, float maxDist) {
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
             RAY_FLAG_SKIP_CLOSEST_HIT_SHADER> q;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = EPSILON;
    ray.TMax = maxDist;

    q.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFF, ray);

    while (q.Proceed()) {
        if (q.CandidateType() == CANDIDATE_PROCEDURAL_PRIMITIVE) {
            uint geomIdx = q.CandidateGeometryIndex();
            uint primIdx = q.CandidatePrimitiveIndex();

            if (geomIdx == 0) {
                // Ground plane: ray-plane at y=0
                float3 o = q.CandidateObjectRayOrigin();
                float3 d = q.CandidateObjectRayDirection();
                if (abs(d.y) > 1e-8) {
                    float t = -o.y / d.y;
                    if (t >= EPSILON && t <= q.CommittedRayT()) {
                        q.CommitProceduralPrimitiveHit(t);
                    }
                }
            } else {
                // Sphere test
                GPUSphereData sphere = g_Spheres[primIdx];
                float3 oc = q.CandidateObjectRayOrigin() - sphere.center;
                float3 d = q.CandidateObjectRayDirection();
                float B = 2.0 * dot(d, oc);
                float C = dot(oc, oc) - sphere.radius * sphere.radius;
                float disc = B * B - 4.0 * C;
                if (disc >= 0) {
                    float sqrtDisc = sqrt(disc);
                    float t1 = 0.5 * (-B - sqrtDisc);
                    float t2 = 0.5 * (-B + sqrtDisc);
                    float t = (t1 >= EPSILON) ? t1 : t2;
                    if (t >= EPSILON) {
                        q.CommitProceduralPrimitiveHit(t);
                    }
                }
            }
        }
    }

    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ||
           q.CommittedStatus() == COMMITTED_PROCEDURAL_PRIMITIVE_HIT;
}

// Ambient occlusion test
bool isAOOccluded(float3 origin, float3 direction, float maxDist) {
    return isOccluded(origin, direction, maxDist);
}
