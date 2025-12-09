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
    float3 Bitangent : BITANGENT;
    float2 TexCoord  : TEXCOORD4;
    float3 Color     : COLOR0;
};

float3 RotateY(float3 p, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    float3 r;
    r.x = p.x * c + p.z * s;
    r.y = p.y;
    r.z = -p.x * s + p.z * c;

    return r;
}

PSInput main(VSInput input)
{
    PSInput output;

    float angle = gTime;
    float3 localPos = RotateY(input.Position, angle);

    float4 worldPos4 = mul(gWorldMatrix, float4(localPos, 1.0f));
    float3 worldPos  = worldPos4.xyz;

    float4 viewPos4 = mul(gViewMatrix, worldPos4);
    float4 clipPos  = mul(gProjectionMatrix, viewPos4);

    output.Position = clipPos;
    output.WorldPos = worldPos;

    float3 worldNormal =
        mul(gWorldMatrix, float4(input.Normal,    0.0f)).xyz;
    float3 worldTangent =
        mul(gWorldMatrix, float4(input.Tangent,   0.0f)).xyz;
    float3 worldBitangent =
        mul(gWorldMatrix, float4(input.Bitangent, 0.0f)).xyz;

    output.Normal    = normalize(worldNormal);
    output.Tangent   = normalize(worldTangent);
    output.Bitangent = normalize(worldBitangent);

    output.TexCoord = input.TexCoord;
    output.Color    = input.Color;

    return output;
}
