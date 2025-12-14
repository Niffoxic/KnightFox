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

struct VSInput
{
    float3 Position   : POSITION;
    float3 Normal     : NORMAL;
    float3 Tangent    : TANGENT;
    float3 Bitangent  : BITANGENT;
    float2 TexCoord0  : TEXCOORD0;
    float2 TexCoord1  : TEXCOORD1;
};

struct PSInput
{
    float4 Position    : SV_POSITION;

    float3 WorldPos    : TEXCOORD0;

    float3 WorldNormal : TEXCOORD1;
    float3 WorldTangent: TEXCOORD2;
    float3 WorldBitan  : TEXCOORD3;

    float2 TexCoord0   : TEXCOORD4;
    float2 TexCoord1   : TEXCOORD5;
};

PSInput main(VSInput input)
{
    PSInput output;

    float4 localPos  = float4(input.Position, 1.0f);

    float4 worldPos4 = mul(localPos,  gWorldMatrix);
    float4 viewPos4  = mul(worldPos4, gViewMatrix);
    float4 clipPos4  = mul(viewPos4,  gProjectionMatrix);

    output.Position = clipPos4;
    output.WorldPos = worldPos4.xyz;

    float3x3 W = (float3x3)gWorldMatrix;

    float3 N = normalize(mul(input.Normal,  W));
    float3 T = normalize(mul(input.Tangent, W));

    T = normalize(T - N * dot(T, N));
    float3 B = normalize(cross(N, T));

    output.WorldNormal  = N;
    output.WorldTangent = T;
    output.WorldBitan   = B;

    output.TexCoord0 = input.TexCoord0;
    output.TexCoord1 = input.TexCoord1;

    return output;
}
