cbuffer CommonCB : register(b0)
{
    //~ Matrices
    float4x4 gWorldMatrix;
    float4x4 gWorldInvTranspose;
    float4x4 gViewMatrix;
    float4x4 gProjectionMatrix;
    float4x4 gOrthogonalMatrix;

    //~ Viewport
    float2   gResolution;
    float2   gInvResolution;

    //~ Mouse
    float2   gMousePosPixels;
    float2   gMousePosNDC;

    //~ Object
    float3   gObjectPosition;
    float    _PadObject;

    //~ Camera
    float3   gCameraPosition;
    float    _PadCamera;

    //~ Player
    float3   gPlayerPosition;
    float    _PadPlayer;

    //~ Time
    float4   gTimeData;     // x=time, y=delta, z=znear, w=zfar
    uint     gFrameIndex;
    uint3    _PadFrame;
};
