cbuffer CommonCB : register(b0)
{
    float4x4 gWorldMatrix;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float4x4 gOrthogonalMatrix;

    float2   gResolution;
    float2   gMousePosition;

    float3   gObjectPosition;
    float    _PaddingObjectPos;

    float3   gCameraPosition;
    float    _PaddingCameraPos;

    float3   gPlayerPosition;
    float    _PaddingPlayerPos;

    float    gTime;
    uint     gFrameIndex;
    float    gDeltaTime;
    float    gZNear;
    float    gZFar;

    float3   _PaddingFinal;
};

struct BaseColorTexture
{
    float IsTextureAttached;
    float UvTilingX;
    float UvTilingY;
    float Strength;
};

struct NormalTexture
{
    float IsTextureAttached;
    float NormalStrength;
    float UvTilingX;
    float UvTilingY;
};

struct ORMTexture
{
    float IsTextureAttached;
    float IsMixed;
    float UvTilingX;
    float UvTilingY;
};

struct EmissiveTexture
{
    float IsTextureAttached;
    float EmissiveIntensity;
    float UvTilingX;
    float UvTilingY;
};

struct OpacityTexture
{
    float IsTextureAttached;
    float AlphaMultiplier;
    float AlphaCutoff;
    float _Pad0;
};

struct HeightTexture
{
    float IsTextureAttached;
    float HeightScale;
    float ParallaxMinLayers;
    float ParallaxMaxLayers;
};

struct SingularOccRoughMetal
{
    float IsOcclusionAttached;
    float IsRoughnessAttached;
    float IsMetallicAttached;
    float _Pad0;

    float OcclusionStrength;
    float RoughnessValue;
    float MetallicValue;
    float _Pad1;

    float OcclusionTilingX;
    float OcclusionTilingY;
    float RoughnessTilingX;
    float RoughnessTilingY;

    float MetallicTilingX;
    float MetallicTilingY;
    float _Pad2;
    float _Pad3;
};

struct Dye
{
    float IsEnabled;
    float Strength;
    float _Pad0;
    float _Pad1;

    float3 Color;
    float _Pad2;
};

cbuffer TextureMetaCB : register(b1)
{
    BaseColorTexture        BaseColor;
    NormalTexture           Normal;
    ORMTexture              ORM;
    EmissiveTexture         Emissive;
    OpacityTexture          Opacity;
    HeightTexture           Height;
    SingularOccRoughMetal   Singular;
    Dye                     DyeParams;

    float ForcedMipLevel;
    float UseForcedMip;
    float _PadMeta0;
    float _PadMeta1;
};

cbuffer DirectionalLightCB : register(b2)
{
    float3 gDirLight_DirectionWS;
    float  gDirLight_Intensity;
    float3 gDirLight_Color;           
    float  gDirLight_ShadowStrength;

    float4x4 gDirLight_LightView;
    float4x4 gDirLight_LightProj;
    float4x4 gDirLight_LightViewProj;

    float    gDirLight_ShadowBias;
    float    gDirLight_NormalBias;
    float    gDirLight_ShadowDistance;
    float    gDirLight_OrthoSize;

    float2   gDirLight_InvShadowMapSize;
    float    gDirLight__PaddingDL0;
    float    gDirLight__PaddingDL1;
}

//~ Sampler
SamplerState gSampler : register(s0);

//~ Inputs
struct PSInput
{
    float4 Position     : SV_POSITION;
    float3 WorldPos     : TEXCOORD0;

    float3 WorldNormal  : TEXCOORD1;
    float3 WorldTangent : TEXCOORD2;
    float3 WorldBitan   : TEXCOORD3;

    float2 TexCoord0    : TEXCOORD4;
    float2 TexCoord1    : TEXCOORD5;
};

//~ Textures
Texture2D gBaseColorTex : register(t0);
Texture2D gNormalTex    : register(t1);
Texture2D gORMTex       : register(t2);
Texture2D gEmissiveTex  : register(t3);
Texture2D gOpacityTex   : register(t4);
Texture2D gHeightTex    : register(t5);

Texture2D gOcclusionTex : register(t6);
Texture2D gRoughnessTex : register(t7);
Texture2D gMetallicTex  : register(t8);

Texture2D gDyeMaskTex   : register(t9);

static const float KFE_EPS = 1e-6f;

float Has(float v) { return step(0.5f, v); }

float4 Sample2D(Texture2D t, float2 uv)
{
    const float useForced = Has(UseForcedMip);
    return lerp(t.Sample(gSampler, uv), t.SampleLevel(gSampler, uv, ForcedMipLevel), useForced);
}

float3 Sample2D_RGB(Texture2D t, float2 uv) { return Sample2D(t, uv).rgb; }
float  Sample2D_R(Texture2D t, float2 uv)   { return Sample2D(t, uv).r; }

float2 GetTiledUV(float2 uv0, float tilingX, float tilingY)
{
    return uv0 * float2(tilingX, tilingY);
}

float3x3 BuildTBN(float3 N, float3 T, float3 B)
{
    N = normalize(N);
    T = normalize(T);
    B = normalize(B);
    return float3x3(T, B, N);
}

float4 SampleBaseColor(float2 uv0)
{
    float2 uv = GetTiledUV(uv0, BaseColor.UvTilingX, BaseColor.UvTilingY);
    float  has = Has(BaseColor.IsTextureAttached);

    float4 tex = Sample2D(gBaseColorTex, uv);
    tex.rgb *= max(BaseColor.Strength, 0.0f);

    return lerp(float4(1,1,1,1), tex, has);
}

float3 SampleNormalWS(PSInput input, float2 uv0)
{
    float2 uv = GetTiledUV(uv0, Normal.UvTilingX, Normal.UvTilingY);
    float  has = Has(Normal.IsTextureAttached);

    float3 nTS = Sample2D_RGB(gNormalTex, uv) * 2.0f - 1.0f;
    nTS.xy *= max(Normal.NormalStrength, 0.0f);
    nTS = normalize(nTS);

    float3x3 TBN = BuildTBN(input.WorldNormal, input.WorldTangent, input.WorldBitan);
    float3 nWS_fromMap = normalize(mul(nTS, TBN));

    float3 nWS = normalize(input.WorldNormal);
    return normalize(lerp(nWS, nWS_fromMap, has));
}

float SampleOcclusionSingle(float2 uv0)
{
    const float hasOcc = Has(Singular.IsOcclusionAttached);
    float2 uv = GetTiledUV(uv0, Singular.OcclusionTilingX, Singular.OcclusionTilingY);
    float occ = Sample2D_R(gOcclusionTex, uv);
    occ = lerp(1.0f, occ, saturate(Singular.OcclusionStrength));
    return lerp(1.0f, occ, hasOcc);
}

float SampleRoughnessSingle(float2 uv0)
{
    const float hasR = Has(Singular.IsRoughnessAttached);
    float2 uv = GetTiledUV(uv0, Singular.RoughnessTilingX, Singular.RoughnessTilingY);
    float rTex = Sample2D_R(gRoughnessTex, uv);
    float r = lerp(Singular.RoughnessValue, rTex, hasR);

    return saturate(r);
}

float SampleMetallicSingle(float2 uv0)
{
    float2 uv = GetTiledUV(uv0, Singular.MetallicTilingX, Singular.MetallicTilingY);

    float mTex = Sample2D_R(gMetallicTex, uv);

    float m = lerp(1.0f, mTex, Has(Singular.IsMetallicAttached));
    m *= Singular.MetallicValue;

    return saturate(m);
}

float SampleOpacityAlpha(float2 uv0)
{
    const float hasA = Has(Opacity.IsTextureAttached);

    float aTex = Sample2D_R(gOpacityTex, uv0);
    float a = lerp(1.0f, aTex, hasA);

    //~ Strength
    a *= Opacity.AlphaMultiplier;

    return saturate(a);
}

float4 main(PSInput input) : SV_TARGET
{
    float2 uv0 = input.TexCoord0;

    const float hasBase   = Has(BaseColor.IsTextureAttached);
    const float hasNormal = Has(Normal.IsTextureAttached);
    const float hasOcc    = Has(Singular.IsOcclusionAttached);
    const float hasRough  = Has(Singular.IsRoughnessAttached);
    const float hasMet    = Has(Singular.IsMetallicAttached);
    const float hasAlpha  = Has(Opacity.IsTextureAttached);

    //~ If literally nothing is attached return dye color
    if ((hasBase + hasNormal + hasOcc + hasRough + hasMet + hasAlpha) < 0.5f)
        return float4(DyeParams.Color, 1.0f);

    //~ Hair alpha
    float alpha = SampleOpacityAlpha(uv0);

    if (hasAlpha > 0.5f)
        clip(alpha - Opacity.AlphaCutoff);

    //~ Base color
    float3 baseColor = DyeParams.Color;
    if (hasBase > 0.5f)
        baseColor = SampleBaseColor(uv0).rgb;

    //~ Normal
    float3 N = normalize(input.WorldNormal);
    if (hasNormal > 0.5f)
        N = SampleNormalWS(input, uv0);

    //~ Directional light (from CB)
    float3 L = normalize(-gDirLight_DirectionWS);
    float3 lightCol = gDirLight_Color * gDirLight_Intensity;

    float NdotL = saturate(dot(N, L));

    const float ambient = 0.25f;

    //~ Single Occlusion affects ambient only
    float occ = SampleOcclusionSingle(uv0);

    //~ Roughness
    float r = SampleRoughnessSingle(uv0);
    float rVis = pow(saturate(r), 0.5f);

    float3 V = normalize(gCameraPosition - input.WorldPos);
    float3 H = normalize(L + V);

    float specPower = lerp(128.0f, 4.0f, rVis);
    float spec = pow(saturate(dot(N, H)), specPower);

    float specIntensity = lerp(1.0f, 0.05f, rVis);
    spec *= specIntensity;

    //~ Metallic
    float m = SampleMetallicSingle(uv0);

    float3 specColor = lerp(float3(1.0f, 1.0f, 1.0f), baseColor, m);
    float  diffuseScale = lerp(1.0f, 0.15f, m);

    //~ AO dampens ambient
    float lighting = (ambient * occ) + (NdotL * diffuseScale);
    float3 diffuse = baseColor * lightCol * lighting;

    float3 specular = specColor * (spec * lightCol);
    specular *= lerp(1.0f, occ, 0.35f);

    float3 finalColor = diffuse + specular;

    return float4(saturate(finalColor), alpha);
}
