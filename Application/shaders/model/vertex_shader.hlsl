cbuffer CommonCB : register(b0)
{
    float4x4 gWorldT;
    float4x4 gWorldInvTransposeT;

    float4x4 gViewT;
    float4x4 gProjT;
    float4x4 gViewProjT;
    float4x4 gOrthoT;

    float3 gCameraPosWS;
    float  gCameraNear;

    float3 gCameraForwardWS;
    float  gCameraFar;

    float3 gCameraRightWS;
    float  _PadCamRight;

    float3 gCameraUpWS;
    float  _PadCamUp;

    float3 gObjectPosWS;
    float  _PadObjPos;

    float3 gPlayerPosWS;
    float  _PadPlayerPos;

    float2 gResolution;
    float2 gInvResolution;

    float2 gMousePosPixels;
    float2 gMousePosNDC;

    float  gTime;
    float  gDeltaTime;
    float  _PadTime0;
    float  _PadTime1;

    uint   gNumTotalLights;
    uint   gRenderFlags;
    uint   _PadFlags0;
    uint   _PadFlags1;
};

struct VSInput
{
    float3 Position  : POSITION; 
    float3 Normal    : NORMAL;  
    float3 Tangent   : TANGENT;  
    float3 Bitangent : BITANGENT; 
    float2 TexCoord0 : TEXCOORD0; 
    float2 TexCoord1 : TEXCOORD1;
};

struct VSOutput
{
    float4 PositionCS   : SV_POSITION;

    float3 WorldPos     : TEXCOORD0;
    float3 WorldNormal  : TEXCOORD1;
    float3 WorldTangent : TEXCOORD2;
    float3 WorldBitan   : TEXCOORD3;

    float2 TexCoord0    : TEXCOORD4;
    float2 TexCoord1    : TEXCOORD5;
};

VSOutput main(VSInput v)
{
    VSOutput o;

    float4 worldPos = mul(float4(v.Position, 1.0f), gWorldT);
    float4 viewPos  = mul(worldPos, gViewT);
    o.PositionCS    = mul(viewPos, gProjT);

    o.WorldPos = worldPos.xyz;

    o.WorldNormal  = normalize(v.Normal);
    o.WorldTangent = normalize(v.Tangent);
    o.WorldBitan   = normalize(v.Bitangent);

    o.TexCoord0 = v.TexCoord0;
    o.TexCoord1 = v.TexCoord1;

    return o;
}
