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

cbuffer TextureMetaCB : register(b1)
{
    float4 gMainTextureInfo;      //~ IsAttached, LerpToSecondary, UvTilingX, UvTilingY
    float4 gSecondaryTextureInfo; //~ IsAttached, BlendFactor,   UvTilingX, UvTilingY
    float4 gNormalTextureInfo;    //~ IsAttached, NormalStrength, UvTilingX, UvTilingY
    float4 gSpecularTextureInfo;  //~ IsAttached, SpecInt, RoughMul, MetalMul
    float4 gHeightTextureInfo;    //~ IsAttached, HeightScale, MinLayers, MaxLayers

    float  gForceMipLevel;        //~ which mip to use (0 = full res)
    float  gUseForcedMip;         //~ 0 = off, 1 = force mip for debug
    float2 _PaddingMip;
};

//~ Texture layout must match ECubeTextures + root signature:
//~ MainTexture      -> t0
//~ SecondaryTexture -> t1
//~ Normal           -> t2
//~ Specular         -> t3
//~ Height           -> t4

Texture2D gMainTexture      : register(t0);
Texture2D gSecondaryTexture : register(t1);
Texture2D gNormalMap        : register(t2);
Texture2D gSpecularMap      : register(t3);
Texture2D gHeightMap        : register(t4);

SamplerState gSampler       : register(s0);

struct PSInput
{
    float4 Position  : SV_POSITION;
    float3 WorldPos  : TEXCOORD0;
    float3 Normal    : TEXCOORD1;
    float3 Tangent   : TEXCOORD2;
    float3 Bitangent : BITANGENT;
    float2 TexCoord  : TEXCOORD4;
    float3 Color     : COLOR0;
};

float4 GetBaseColor(float2 uv)
{
    float mainAttached = saturate(gMainTextureInfo.x);
    float secAttached  = saturate(gSecondaryTextureInfo.x);

    float2 mainUV = uv * gMainTextureInfo.zw;
    float2 secUV  = uv * gSecondaryTextureInfo.zw;

    float4 mainSample;
    float4 secSample;

    if (gUseForcedMip > 0.5f)
    {
        mainSample = gMainTexture.SampleLevel(gSampler,      mainUV, gForceMipLevel);
        secSample  = gSecondaryTexture.SampleLevel(gSampler, secUV,  gForceMipLevel);
    }
    else
    {
        mainSample = gMainTexture.Sample(gSampler,      mainUV);
        secSample  = gSecondaryTexture.Sample(gSampler, secUV);
    }

    float3 mainRgb = mainSample.rgb;
    float3 secRgb  = secSample.rgb;

    float hasMain = step(0.5f, mainAttached);
    float hasSec  = step(0.5f, secAttached);

    float wNone = (1.0f - hasMain) * (1.0f - hasSec);
    float wMain = hasMain * (1.0f - hasSec);
    float wSec  = (1.0f - hasMain) * hasSec;
    float wBoth = hasMain * hasSec;

    float lerpToSecondary = saturate(gMainTextureInfo.y);
    float secBlendFactor  = saturate(gSecondaryTextureInfo.y);

    //~ first mix main → secondary using secondary blend factor
    float3 secMix  = lerp(mainRgb,      secRgb,      secBlendFactor);
    float  secMixA = lerp(mainSample.a, secSample.a, secBlendFactor);

    //~ now main lerps toward that mixed result using LerpToSecondary
    float3 bothColor = lerp(mainRgb,      secMix,  lerpToSecondary);
    float  bothAlpha = lerp(mainSample.a, secMixA, lerpToSecondary);

    float3 noneColor = float3(1.0f, 0.0f, 1.0f);
    float  noneAlpha = 1.0f;

    float3 blendedRgb =
        wNone * noneColor +
        wMain * mainRgb +
        wSec  * secRgb +
        wBoth * bothColor;

    float blendedAlpha =
        wNone * noneAlpha +
        wMain * mainSample.a +
        wSec  * secSample.a +
        wBoth * bothAlpha;

    return float4(blendedRgb, blendedAlpha);
}

float3 GetNormalWS(PSInput input)
{
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Bitangent);
    float3 N = normalize(input.Normal);

    float3x3 TBN = float3x3(T, B, N);

    float normalAttached = saturate(gNormalTextureInfo.x);
    float normalStrength = gNormalTextureInfo.y;

    float2 normalUV = input.TexCoord * gNormalTextureInfo.zw;

    float3 nTS;
    if (gUseForcedMip > 0.5f)
    {
        nTS = gNormalMap.SampleLevel(gSampler, normalUV, gForceMipLevel).xyz;
    }
    else
    {
        nTS = gNormalMap.Sample(gSampler, normalUV).xyz;
    }

    nTS = nTS * 2.0f - 1.0f;

    float3 defaultTS = float3(0.0f, 0.0f, 1.0f);

    float strengthFactor = saturate(normalAttached * normalStrength);

    nTS = normalize(lerp(defaultTS, nTS, strengthFactor));
    float3 nWS = normalize(mul(nTS, TBN));

    return nWS;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 baseColor = GetBaseColor(input.TexCoord);
    float3 normalWS  = GetNormalWS(input);

    float lightFactor = 0.5f + 0.5f * normalWS.z;
    float3 finalRgb   = baseColor.rgb * lightFactor;

    return float4(finalRgb, baseColor.a);
}
