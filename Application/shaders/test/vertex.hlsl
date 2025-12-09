cbuffer CommonCB : register(b0)
{
    float4x4 gWorldMatrix;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;

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

struct VSInput
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 TexCoord  : TEXCOORD0;
    float3 Color     : COLOR0;
};

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

PSInput main(VSInput input)
{
    PSInput output;
    float3 worldPos = input.Position;
    float3 camPos  = float3(0.0f, 0.0f, -5.0f);
    float3 viewPos = worldPos - camPos;
    float z = max(viewPos.z, 0.1f);
    float fovFactor = 1.5f;
    float2 ndcXY = (viewPos.xy / (z * fovFactor));
    output.Position = float4(ndcXY, 0.0f, 1.0f);

    output.WorldPos  = worldPos;
    output.Normal    = input.Normal;
    output.Tangent   = input.Tangent;
    output.Bitangent = input.Bitangent;
    output.TexCoord  = input.TexCoord;
    output.Color     = input.Color;

    return output;
}