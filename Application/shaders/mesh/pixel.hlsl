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

struct PSInput
{
    float4 Position   : SV_POSITION;
    float3 WorldPos   : TEXCOORD0;
    float3 Normal     : TEXCOORD1;
    float2 TexCoord0  : TEXCOORD2;
    float2 TexCoord1  : TEXCOORD3;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.Normal);

    float3 lightDir = normalize(float3(0.5f, -1.0f, 0.25f));
    float  NdotL    = saturate(dot(N, -lightDir));

    float3 baseColor = float3(0.8f, 0.75f, 0.7f);
    float3 color     = baseColor * (0.2f + 0.8f * NdotL);

    return float4(color, 1.0f);
}
