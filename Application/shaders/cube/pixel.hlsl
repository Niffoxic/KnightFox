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

Texture2D    gDiffuseTexture : register(t0);
SamplerState gSampler        : register(s0);

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
    float4 texColor    = gDiffuseTexture.Sample(gSampler, input.TexCoord);
    float3 tintedColor = texColor.rgb * input.Color;
    float3 final       = lerp(tintedColor * 0.2f, tintedColor, 1.f);

    return float4(final, texColor.a);
}
