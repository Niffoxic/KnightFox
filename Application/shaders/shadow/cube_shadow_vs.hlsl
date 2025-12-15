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

cbuffer DirectionalLightCB : register(b1)
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

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput main(VSInput input)
{
    VSOutput o;

    float4 worldPos = mul(gWorldMatrix, float4(input.Position, 1.0f));
    o.Position      = mul(gDirLight_LightViewProj, worldPos);
    return o;
}
