cbuffer CommonCB : register(b0)
{
    //~ Matrices
    float4x4 gWorldMatrix;
    float4x4 gWorldInvTranspose;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float4x4 gOrthogonalMatrix;

    //~ Viewport
    float2   gResolution;
    float2   gInvResolution;

    //~ Mouse
    float2   gMousePosPixels;
    float2   gMousePosNDC;

    //~ Object
    float3   gObjectPosition;
    float    _PadObject;

    //~ Camera
    float3   gCameraPosition;
    float    _PadCamera;

    //~ Player
    float3   gPlayerPosition;
    float    _PadPlayer;

    //~ Time
    float4   gTimeData;     // x=time, y=delta, z=znear, w=zfar
    uint     gFrameIndex;
    uint3    _PadFrame;
};

cbuffer TextureMetaCB : register(b1)
{
    // BaseColor
    float4 BaseColor_Meta; // x=IsAttached y=UvX z=UvY w=Strength

    // Normal
    float4 Normal_Meta;    // x=IsAttached y=Strength z=UvX w=UvY

    // ORM
    float4 ORM_Meta;       // x=IsAttached y=IsMixed z=UvX w=UvY

    // Emissive
    float4 Emissive_Meta;  // x=IsAttached y=Intensity z=UvX w=UvY

    // Opacity
    float4 Opacity_Meta;   // x=IsAttached y=AlphaMul z=AlphaCutoff w=pad

    // Height
    float4 Height_Meta;    // x=IsAttached y=HeightScale z=MinLayers w=MaxLayers

    // Displacement
    float4 Displace_Meta;  // x=IsAttached y=Scale z=UvX w=UvY

    // Specular
    float4 Specular_Meta;  // x=IsAttached y=Strength z=UvX w=UvY

    // Glossiness
    float4 Gloss_Meta;     // x=IsAttached y=Strength z=UvX w=UvY

    // DetailNormal
    float4 DetailN_Meta;   // x=IsAttached y=Strength z=UvX w=UvY

    // Singular packed set (16 floats)
    float4 Singular0; // x=IsOccAttached y=IsRoughAttached z=IsMetalAttached w=pad
    float4 Singular1; // x=OccStrength y=RoughValue z=MetalValue w=pad
    float4 Singular2; // x=OccUvX y=OccUvY z=RoughUvX w=RoughUvY
    float4 Singular3; // x=MetalUvX y=MetalUvY z=pad w=pad

    // Forced mip
    float4 ForcedMip; // x=ForcedMipLevel y=UseForcedMip z=pad w=pad
};

Texture2D gBaseColorTex     : register(t0);  // BaseColor
Texture2D gNormalTex        : register(t1);  // Normal
Texture2D gORMTex           : register(t2);  // ORM
Texture2D gEmissiveTex      : register(t3);  // Emissive

Texture2D gRoughnessTex     : register(t4);  // Roughness
Texture2D gMetallicTex      : register(t5);  // Metallic
Texture2D gOcclusionTex     : register(t6);  // Occlusion

Texture2D gOpacityTex       : register(t7);  // Opacity

Texture2D gHeightTex        : register(t8);  // Height
Texture2D gDisplacementTex  : register(t9);  // Displacement

Texture2D gSpecularTex      : register(t10); // Specular
Texture2D gGlossinessTex    : register(t11); // Glossiness

Texture2D gDetailNormalTex  : register(t12); // DetailNormal
Texture2D gShadowMapTex     : register(t13); // ShadowMap

SamplerState gSamp0 : register(s0);
SamplerComparisonState gShadowCmp : register(s1);

struct KFE_LIGHT_DATA_GPU
{
    uint  Type;
    uint  Flags;
    uint  ShadowTech;
    uint  ShadowMapId;

    float Intensity;
    float Range;
    float Attenuation;
    float ShadowStrength;

    float3 PositionWS; float _PadPos;

    float3 DirectionWS; float _PadDir0;
    float3 DirectionWSNormalized; float _PadDir1;

    float3 Color; float _PadColor;

    float SpotInnerCos;
    float SpotOuterCos;
    float SpotSoftness;
    float _PadSpot;

    float ShadowBias;
    float NormalBias;
    float ShadowNearZ;
    float ShadowFarZ;

    float ShadowFilterRadius;
    float ShadowTexelSize;
    float ShadowFadeStart;
    float ShadowFadeEnd;

    float2 ShadowUVScale;
    float2 ShadowUVOffset;

    float4   CascadeSplits;
    float4x4 CascadeViewProjT[4];

    float4x4 LightViewProjT;

    float4x4 PointFaceViewProjT[6];

    float2 InvShadowMapSize;
    float _PadSM0;
    float _PadSM1;
};

StructuredBuffer<KFE_LIGHT_DATA_GPU> gLights : register(t15);

float HasTex(float flag) { return step(0.5f, flag); }

float3 ApplyBaseColorTex(float2 uv0, float3 fallbackColor)
{
    const float has = HasTex(BaseColor_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * BaseColor_Meta.yz;

    // Normal sampling
    const float3 texSample =
        gBaseColorTex.Sample(gSamp0, uv).rgb;

    // Forced mip
    const float3 texForced =
        gBaseColorTex.SampleLevel(gSamp0, uv, ForcedMip.x).rgb;

    const float useForced = step(0.5f, ForcedMip.y);
    const float3 tex =
        lerp(texSample, texForced, useForced);

    // Strength blend
    const float strength = saturate(BaseColor_Meta.w);

    // Final combine
    return lerp(
        fallbackColor,
        lerp(fallbackColor, tex, strength),
        enable
    );
}

float3 ApplyNormalTex(
    float2 uv0,
    float3 worldN,
    float3 worldT,
    float3 worldB)
{
    const float has    = HasTex(Normal_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * Normal_Meta.zw;

    // Sample normal map
    const float3 s0 = gNormalTex.Sample(gSamp0, uv).xyz;
    const float3 sF = gNormalTex.SampleLevel(gSamp0, uv, ForcedMip.x).xyz;

    const float useForced = step(0.5f, ForcedMip.y);
    const float3 sample01 = lerp(s0, sF, useForced);

    // Unpack
    float3 nTS = sample01 * 2.0f - 1.0f;
    nTS = normalize(nTS);

    // Build TBN
    const float3 N = normalize(worldN);
    const float3 T = normalize(worldT);
    const float3 B = normalize(worldB);

    float3 nWS = normalize(nTS.x * T + nTS.y * B + nTS.z * N);

    // Strength blend
    const float strength = saturate(Normal_Meta.y);
    nWS = normalize(lerp(N, nWS, strength));

    return normalize(lerp(N, nWS, enable));
}

float3 ApplyFakeLighting(
    float3 baseColor,
    float3 worldN,
    float3 worldPos)
{
    const float3 lightDir = normalize(float3(0.3f, 1.0f, 0.2f));
    const float NdotL = saturate(dot(normalize(worldN), lightDir));
    const float ambient = 0.25f;
    return baseColor * (ambient + NdotL);
}

float ApplyOcclusionTex(float2 uv0)
{
    const float has    = HasTex(Singular0.x);
    const float enable = has;

    const float2 uv = uv0 * Singular2.xy;

    // Sample AO
    const float aoSample =
        gOcclusionTex.Sample(gSamp0, uv).r;

    // Forced mip
    const float aoForced =
        gOcclusionTex.SampleLevel(gSamp0, uv, ForcedMip.x).r;

    const float useForced = step(0.5f, ForcedMip.y);
    const float aoTex = lerp(aoSample, aoForced, useForced);

    // Strength
    const float strength = saturate(Singular1.x);

    // Blend AO
    const float ao = lerp(1.0f, aoTex, strength);
    return lerp(1.0f, ao, enable);
}

float2 ApplyDisplacementUV(float2 uv0, float3 worldPos)
{
    const float has    = HasTex(Displace_Meta.x);
    const float enable = has;

    const float2 uvD = uv0 * Displace_Meta.zw;

    const float h0 = gDisplacementTex.Sample(gSamp0, uvD).r;
    const float hF = gDisplacementTex.SampleLevel(gSamp0, uvD, ForcedMip.x).r;

    const float useForced = step(0.5f, ForcedMip.y);
    const float h = lerp(h0, hF, useForced);

    // Center around 0
    const float height = (h - 0.5f) * Displace_Meta.y;

    // View direction in world
    const float3 V = normalize(gCameraPosition - worldPos);

    const float2 offset = V.xy * height;

    const float2 uvOut = uv0 + offset;

    return lerp(uv0, uvOut, enable);
}

float SampleGloss(float2 uv0)
{
    const float has    = HasTex(Gloss_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * Gloss_Meta.zw;

    const float g0 = gGlossinessTex.Sample(gSamp0, uv).r;
    const float gF = gGlossinessTex.SampleLevel(gSamp0, uv, ForcedMip.x).r;

    const float useForced = step(0.5f, ForcedMip.y);
    float gloss = lerp(g0, gF, useForced);

    gloss *= saturate(Gloss_Meta.y);
    return lerp(0.0f, gloss, enable);
}

float3 ApplyFakeSpecular(
    float3 litColor,
    float3 worldN,
    float3 worldPos,
    float gloss01)
{
    const float3 N = normalize(worldN);
    // Same fake light dir
    const float3 L = normalize(float3(0.3f, 1.0f, 0.2f));
    // View direction
    const float3 V = normalize(gCameraPosition - worldPos);
    const float3 H = normalize(L + V);
    const float shininess = lerp(4.0f, 256.0f, saturate(gloss01));
    const float NdotH = saturate(dot(N, H));
    const float spec = pow(NdotH, shininess);
    return litColor + spec * gloss01;
}

void ApplyOpacityCutout(float2 uv0)
{
    const float has = HasTex(Opacity_Meta.x);
    if (has < 0.5f) return;

    const float a0 = gOpacityTex.Sample(gSamp0, uv0).r;
    const float aF = gOpacityTex.SampleLevel(gSamp0, uv0, ForcedMip.x).r;

    const float useForced = step(0.5f, ForcedMip.y);
    float a = lerp(a0, aF, useForced);

    a *= Opacity_Meta.y;
    clip(a - Opacity_Meta.z);
}
