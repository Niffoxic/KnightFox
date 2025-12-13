//~ -------------------------
//~ b0: Common per-object/per-frame (you already have this)
//~ -------------------------
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

//~ -------------------------
//~ b1: Per-submesh texture meta (your struct layout)
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
    BaseColorTexture base;
    NormalTexture           Normal;
    ORMTexture              ORM;
    EmissiveTexture         Emissive;
    OpacityTexture          Opacity;
    HeightTexture           Height;
    SingularOccRoughMetal   Singular;
    Dye                     ColorDyes;

    float ForcedMipLevel;
    float UseForcedMip;
    float _PadMeta0;
    float _PadMeta1;
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

//~ Sampler
SamplerState gSampler : register(s0);

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

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord0;

    if (base.IsTextureAttached > 0.5f)
    {
        float2 tiledUV = uv * float2(base.UvTilingX, base.UvTilingY);
        return gBaseColorTex.Sample(gSampler, tiledUV);
    }

    float t = 1.f - base.IsTextureAttached;
    return float4(t, t, t, 1.f);
}
