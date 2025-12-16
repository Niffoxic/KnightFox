cbuffer CommonCB : register(b0)
{
    row_major float4x4 gWorldT;
    row_major float4x4 gWorldInvTransposeT;

    row_major float4x4 gViewT;
    row_major float4x4 gProjT;
    row_major float4x4 gViewProjT;
    row_major float4x4 gOrthoT;

    float3 gCameraPosWS;      float gCameraNear;
    float3 gCameraForwardWS;  float gCameraFar;
    float3 gCameraRightWS;    float _PadCamRight;
    float3 gCameraUpWS;       float _PadCamUp;

    float3 gObjectPosWS;      float _PadObjPos;
    float3 gPlayerPosWS;      float _PadPlayerPos;

    float2 gResolution;
    float2 gInvResolution;

    float2 gMousePosPixels;
    float2 gMousePosNDC;

    float gTime;
    float gDeltaTime;
    float _PadTime0;
    float _PadTime1;

    uint gNumTotalLights;
    uint gRenderFlags;
    uint _PadFlags0;
    uint _PadFlags1;
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

    // Singular
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

struct KFE_LightData
{
    // header
    float LightType;  // 0 = Spot, 1 = Directional, 2 = Point
    float Intensity;
    float Range;
    float Attenuation;

    float3 PositionWS;
    float  _PadPos;

    float3 DirectionWS;
    float  _PadDir0;

    float3 DirectionWSNormalized;
    float  _PadDir1;

    float3 Color;
    float  _PadColor;

    float SpotInnerCos;
    float SpotOuterCos;
    float SpotSoftness;
    float _PadSpot;
};

StructuredBuffer<KFE_LightData> gLights : register(t15);

float HasTex(float flag) { return step(0.5f, flag); }

float UseForcedMip() { return step(0.5f, ForcedMip.y); }

//~ Sample 2D with optional forced mip
float SampleTex1(Texture2D tex, float2 uv)
{
    const float s0 = tex.Sample(gSamp0, uv).r;
    const float sF = tex.SampleLevel(gSamp0, uv, ForcedMip.x).r;
    return lerp(s0, sF, UseForcedMip());
}

//~ Sample 2D with forced mip in rgb
float3 SampleTex3(Texture2D tex, float2 uv)
{
    const float3 s0 = tex.Sample(gSamp0, uv).rgb;
    const float3 sF = tex.SampleLevel(gSamp0, uv, ForcedMip.x).rgb;
    return lerp(s0, sF, UseForcedMip());
}


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
    const float3 V = normalize(gCameraPosWS - worldPos);

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

//~ Lights
float3 ComputeDirectionalLightsLambert(float3 baseColor, float3 worldN)
{
    const float3 N = normalize(worldN);

    // Small ambient
    const float ambient = 0.20f;
    float3 result = baseColor * ambient;

    const uint n = min(gNumTotalLights, 256u);

    [loop]
    for (uint i = 0; i < n; ++i)
    {
        KFE_LightData L = gLights[i];

        // 0=Spot, 1=Directional, 2=Point
        if (abs(L.LightType - 1.0f) >= 0.25f)
            continue;

        // normalized dir if valid or else normalize raw
        float3 dirN = L.DirectionWSNormalized;
        if (dot(dirN, dirN) < 1e-6f)
            dirN = normalize(L.DirectionWS);

        const float3 Ldir = normalize(-dirN);
        const float NdotL = saturate(dot(N, Ldir));

        const float intensity = max(L.Intensity, 0.0f);
        const float3 lightColor = max(L.Color, 0.0f);

        result += baseColor * (lightColor * intensity) * NdotL;
    }

    result = result / (1.0f + result);

    return result;
}

float ComputePointAttenuation(float dist, float range, float attenuation)
{
    range = max(range, 1e-3f);
    attenuation = max(attenuation, 0.0f);

    float x = saturate(1.0f - (dist / range));
    float rangeFade = x * x;

    float invSq = 1.0f / (1.0f + attenuation * dist * dist);

    return rangeFade * invSq;
}

float3 ComputePointLightsLambert(float3 baseColor, float3 worldN, float3 worldPos)
{
    const float3 N = normalize(worldN);

    float3 result = 0.0f;

    const uint n = min(gNumTotalLights, 256u);

    [loop]
    for (uint i = 0; i < n; ++i)
    {
        KFE_LightData L = gLights[i];

        if (abs(L.LightType - 2.0f) >= 0.25f)
            continue;

        const float3 toLight = (L.PositionWS - worldPos);
        const float distSq = dot(toLight, toLight);

        if (distSq < 1e-8f)
            continue;

        const float dist = sqrt(distSq);
        const float3 Ldir = toLight / dist;

        const float NdotL = saturate(dot(N, Ldir));

        const float intensity = max(L.Intensity, 0.0f);
        const float3 lightColor = max(L.Color, 0.0f);

        const float att = ComputePointAttenuation(dist, L.Range, L.Attenuation);

        result += baseColor * (lightColor * intensity) * (NdotL * att);
    }

    return result;
}

float3 ComputePointLightsDiffuseSpec_BlinnPhong(
    float3 baseColor,
    float3 worldN,
    float3 worldPos,
    float gloss01)
{
    const float3 N = normalize(worldN);
    const float3 V = normalize(gCameraPosWS - worldPos);

    float3 diffuseAcc = 0.0f;
    float3 specAcc    = 0.0f;

    // shininess exponent
    // 0 to broad highlight and 1 -to tight highlight
    const float shininess = lerp(8.0f, 256.0f, saturate(gloss01));

    const uint n = min(gNumTotalLights, 256u);

    [loop]
    for (uint i = 0; i < n; ++i)
    {
        KFE_LightData L = gLights[i];

        // 2 = Point lights
        if (abs(L.LightType - 2.0f) >= 0.25f)
            continue;

        const float3 toLight = (L.PositionWS - worldPos);
        const float distSq = dot(toLight, toLight);
        if (distSq < 1e-8f)
            continue;

        const float dist = sqrt(distSq);
        const float3 Ldir = toLight / dist;

        const float NdotL = saturate(dot(N, Ldir));
        if (NdotL <= 0.0f)
            continue;

        const float att = ComputePointAttenuation(dist, L.Range, L.Attenuation);

        const float intensity = max(L.Intensity, 0.0f);
        const float3 lightColor = max(L.Color, 0.0f);
        const float3 radiance = lightColor * intensity * att;

        // Diffuse Lambert tech
        diffuseAcc += baseColor * radiance * NdotL;

        // Blinn-Phong tech with use half vector H
        const float3 H = normalize(Ldir + V);
        const float NdotH = saturate(dot(N, H));
        const float spec = pow(NdotH, shininess);

        // fades out at 0
        specAcc += radiance * (spec * saturate(gloss01));
    }

    return diffuseAcc + specAcc;
}

float ComputeSpotConeFactor(float3 Ldir_SurfaceToLight, float3 spotDirN, float innerCos, float outerCos, float softness01)
{
    // inner should be tighter
    const float ic = max(innerCos, outerCos);
    const float oc = min(innerCos, outerCos);

    // angle between the spot forward and direction from the light to suface
    const float3 lightToSurf = -Ldir_SurfaceToLight;

    const float cd = dot(normalize(spotDirN), normalize(lightToSurf));

    // 1 inside inner cone and 0 outside outer cone then smooth between
    float t = saturate((cd - oc) / max(ic - oc, 1e-5f));
    float cone = t;

    // Softness if 0 = linear smoothstep region and 1 = softer
    const float s = saturate(softness01);
    const float p = lerp(1.0f, 0.35f, s);   // TODO: Make if configurable softer edge
    cone = pow(cone, p);

    return cone;
}

float3 ComputeSpotLightsDiffuseSpec_BlinnPhong(
    float3 baseColor,
    float3 worldN,
    float3 worldPos,
    float gloss01)
{
    const float3 N = normalize(worldN);
    const float3 V = normalize(gCameraPosWS - worldPos);

    float3 diffuseAcc = 0.0f;
    float3 specAcc    = 0.0f;

    const float shininess = lerp(8.0f, 256.0f, saturate(gloss01));

    const uint n = min(gNumTotalLights, 256u);

    [loop]
    for (uint i = 0; i < n; ++i)
    {
        KFE_LightData L = gLights[i];

        // 0 for Spot lighting
        if (abs(L.LightType - 0.0f) >= 0.25f)
            continue;

        const float3 toLight = (L.PositionWS - worldPos);
        const float distSq = dot(toLight, toLight);
        if (distSq < 1e-8f)
            continue;

        const float dist = sqrt(distSq);
        const float3 Ldir = toLight / dist; // surface to light

        const float NdotL = saturate(dot(N, Ldir));
        if (NdotL <= 0.0f)
            continue;

        // Spot direction normalization
        float3 spotDirN = L.DirectionWSNormalized;
        if (dot(spotDirN, spotDirN) < 1e-6f)
            spotDirN = normalize(L.DirectionWS);

        // Cone factor
        const float cone = ComputeSpotConeFactor(
            Ldir,
            spotDirN,
            L.SpotInnerCos,
            L.SpotOuterCos,
            L.SpotSoftness);

        if (cone <= 0.0f)
            continue;

        // Range and distance attenuation
        const float att = ComputePointAttenuation(dist, L.Range, L.Attenuation);

        const float intensity = max(L.Intensity, 0.0f);
        const float3 lightColor = max(L.Color, 0.0f);
        const float3 radiance = lightColor * intensity * att * cone;

        // Diffuse
        diffuseAcc += baseColor * radiance * NdotL;

        // Specular Blinn-Phong tech
        const float3 H = normalize(Ldir + V);
        const float NdotH = saturate(dot(N, H));
        const float spec = pow(NdotH, shininess);

        specAcc += radiance * (spec * saturate(gloss01) * NdotL);
    }

    return diffuseAcc + specAcc;
}

float3 ApplyEmissiveTex(float2 uv0)
{
    const float has    = HasTex(Emissive_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * Emissive_Meta.zw;

    //~ RGB emissive color
    const float3 e = SampleTex3(gEmissiveTex, uv);

    //~ Intensity is like a multiplier
    const float intensity = max(Emissive_Meta.y, 0.0f);

    return e * intensity * enable;
}

float3 SampleORM(float2 uv0)
{
    const float hasOrm = step(0.5f, ORM_Meta.x);
    const float mixed  = step(0.5f, ORM_Meta.y);

    // ORM sample
    const float2 uvOrm  = uv0 * ORM_Meta.zw;
    const float3 ormTex = SampleTex3(gORMTex, uvOrm);

    const float aoTex    = ormTex.r;
    const float roughTex = ormTex.g;
    const float metalTex = ormTex.b;

    // Singular enables
    const float aoSingHas    = step(0.5f, Singular0.x);
    const float roughSingHas = step(0.5f, Singular0.y);
    const float metalSingHas = step(0.5f, Singular0.z);

    // Singular UVs
    const float2 uvAO    = uv0 * Singular2.xy;
    const float2 uvRough = uv0 * Singular2.zw;
    const float2 uvMetal = uv0 * Singular3.xy;

    // Singular samples
    const float aoSingTex    = SampleTex1(gOcclusionTex, uvAO);
    const float roughSingTex = SampleTex1(gRoughnessTex, uvRough);
    const float metalSingTex = SampleTex1(gMetallicTex, uvMetal);

    // Singular constants
    const float aoConst    = 1.0f;
    const float roughConst = saturate(Singular1.y);
    const float metalConst = saturate(Singular1.z);

    // Final singular values
    const float aoSing    = lerp(aoConst,    aoSingTex,    aoSingHas);
    const float roughSing = lerp(roughConst, roughSingTex, roughSingHas);
    const float metalSing = lerp(metalConst, metalSingTex, metalSingHas);

    const float aoFromOrm    = lerp(aoTex,    aoSing,    mixed);
    const float roughFromOrm = lerp(roughTex, roughSing, mixed);
    const float metalFromOrm = lerp(metalTex, metalSing, mixed);

    const float ao    = lerp(aoSing,    aoFromOrm,    hasOrm);
    const float rough = lerp(roughSing, roughFromOrm, hasOrm);
    const float metal = lerp(metalSing, metalFromOrm, hasOrm);

    return saturate(float3(ao, rough, metal));
}

float SampleRoughness(float2 uv0)
{
    const float has    = HasTex(Singular0.y);
    const float enable = has;

    const float2 uv = uv0 * Singular2.zw;

    //~ Texture or constant
    const float rTex = SampleTex1(gRoughnessTex, uv);
    const float rVal = saturate(Singular1.y);

    return lerp(rVal, rTex, enable);
}

float SampleMetallic(float2 uv0)
{
    const float has    = HasTex(Singular0.z);
    const float enable = has;

    const float2 uv = uv0 * Singular3.xy;

    //~ Texture or constant
    const float mTex = SampleTex1(gMetallicTex, uv);
    const float mVal = saturate(Singular1.z);

    return lerp(mVal, mTex, enable);
}

float3 SampleSpecularColor(float2 uv0, float3 fallbackSpec)
{
    const float has    = HasTex(Specular_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * Specular_Meta.zw;

    //~ RGB spec color
    const float3 s = SampleTex3(gSpecularTex, uv);

    //~ Strength mixes toward fallback
    const float strength = saturate(Specular_Meta.y);

    const float3 outS = lerp(fallbackSpec, s, strength);
    return lerp(fallbackSpec, outS, enable);
}

float3 ApplyDetailNormalTex(float2 uv0, float3 baseN, float3 worldT, float3 worldB)
{
    const float has    = HasTex(DetailN_Meta.x);
    const float enable = has;

    const float2 uv = uv0 * DetailN_Meta.zw;

    //~ Detail normal in TS
    const float3 s0 = gDetailNormalTex.Sample(gSamp0, uv).xyz;
    const float3 sF = gDetailNormalTex.SampleLevel(gSamp0, uv, ForcedMip.x).xyz;
    const float3 s  = lerp(s0, sF, UseForcedMip());

    float3 nTS = normalize(s * 2.0f - 1.0f);

    //~ TBN
    const float3 N = normalize(baseN);
    const float3 T = normalize(worldT);
    const float3 B = normalize(worldB);

    float3 nWS = normalize(nTS.x * T + nTS.y * B + nTS.z * N);

    //~ Strength blends detail into base normal
    const float strength = saturate(DetailN_Meta.y);

    return normalize(lerp(N, nWS, strength * enable));
}

float2 ApplyHeightParallaxUV(float2 uv0, float3 worldPos)
{
    const float has    = HasTex(Height_Meta.x);
    const float enable = has;

    //~ Height scale
    const float scale = Height_Meta.y;

    //~ Sample height
    const float h0 = gHeightTex.Sample(gSamp0, uv0).r;
    const float hF = gHeightTex.SampleLevel(gSamp0, uv0, ForcedMip.x).r;
    const float h  = lerp(h0, hF, UseForcedMip());

    //~ Center height at 0
    const float height = (h - 0.5f) * scale;

    //~ View dir in world
    const float3 V = normalize(gCameraPosWS - worldPos);

    const float2 uvOut = uv0 + V.xy * height;

    return lerp(uv0, uvOut, enable);
}
