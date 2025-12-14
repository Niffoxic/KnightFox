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
    float3 Bitangent : TEXCOORD3;
    float2 TexCoord  : TEXCOORD4;
    float3 Color     : COLOR0;
};

PSInput main(VSInput input)
{
    PSInput o;

    float4 localPos  = float4(input.Position, 1.0f);

    float4 worldPos4 = mul(localPos, gWorldMatrix);
    o.WorldPos = worldPos4.xyz;

    float4 viewPos4  = mul(worldPos4, gViewMatrix);
    o.Position        = mul(viewPos4, gProjectionMatrix);

    float3x3 W = (float3x3)gWorldMatrix;

    float3 N = normalize(mul(input.Normal,  W));
    float3 T = normalize(mul(input.Tangent, W));

    T = normalize(T - N * dot(T, N));
    float3 B = normalize(cross(N, T));

    o.Normal    = N;
    o.Tangent   = T;
    o.Bitangent = B;

    o.TexCoord = input.TexCoord;
    o.Color    = input.Color;

    return o;
}
