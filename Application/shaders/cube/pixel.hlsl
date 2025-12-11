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
    float4 gMainTextureInfo;      // IsAttached, LerpToSecondary, UvTilingX, UvTilingY
    float4 gSecondaryTextureInfo; // IsAttached, BlendFactor,   UvTilingX, UvTilingY
    float4 gNormalTextureInfo;    // IsAttached, NormalStrength, UvTilingX, UvTilingY
    float4 gSpecularTextureInfo;  // IsAttached, SpecInt, RoughMul, MetalMul
    float4 gHeightTextureInfo;    // IsAttached, HeightScale, MinLayers, MaxLayers
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

float4 main(PSInput input) : SV_TARGET
{
    // Just a Flags
    float mainAttached = gMainTextureInfo.x;
    float secAttached  = gSecondaryTextureInfo.x;

    float2 mainUV = input.TexCoord * gMainTextureInfo.zw;
    float2 secUV  = input.TexCoord * gSecondaryTextureInfo.zw;

    float4 mainColor      = gMainTexture.Sample(gSampler, mainUV);
    float4 secondaryColor = gSecondaryTexture.Sample(gSampler, secUV);

    float3 mainRgb = mainColor.rgb * mainAttached;
    float3 secRgb  = secondaryColor.rgb * secAttached;

    float lerpToSecondary = gMainTextureInfo.y; 
    float secBlendFactor  = gSecondaryTextureInfo.y;
    float blendT          = saturate(lerpToSecondary * secBlendFactor);

    float hasMain = step(0.5f, mainAttached);
    float hasSec  = step(0.5f, secAttached);

    float3 blendedRgb = 0.0f;

    if (hasMain + hasSec < 0.5f)
    {
        blendedRgb = float3(1.0f, 0.0f, 1.0f);
    }
    else if (hasMain > 0.5f && hasSec < 0.5f)
    {
        blendedRgb = mainRgb;
    }
    else if (hasMain < 0.5f && hasSec > 0.5f)
    {
        blendedRgb = secRgb;
    }
    else
    {
        blendedRgb = lerp(mainRgb, secRgb, blendT);
    }

    return float4(blendedRgb, mainColor.a);
}
