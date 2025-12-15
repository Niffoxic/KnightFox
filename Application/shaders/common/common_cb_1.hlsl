cbuffer CommonCB : register(b0)
{
    row_major float4x4 gWorldT;
    row_major float4x4 gWorldInvTransposeT;

    row_major float4x4 gViewT;
    row_major float4x4 gProjT;
    row_major float4x4 gViewProjT;
    row_major float4x4 gOrthoT;

    float3 gCameraPosWS;      float gCameraNear;
    float3 gCameraForwardWS;  float gCameraFar;
    float3 gCameraRightWS;    float _PadCamRight;
    float3 gCameraUpWS;       float _PadCamUp;

    float3 gObjectPosWS;      float _PadObjPos;
    float3 gPlayerPosWS;      float _PadPlayerPos;

    float2 gResolution;
    float2 gInvResolution;

    float2 gMousePosPixels;
    float2 gMousePosNDC;

    float gTime;
    float gDeltaTime;
    float _PadTime0;
    float _PadTime1;

    uint gNumTotalLights;
    uint gRenderFlags;
    uint _PadFlags0;
    uint _PadFlags1;
};
