cbuffer MipGenCB : register(b0)
{
    uint2 gSrcSize;
    uint2 gDstSize;
};

Texture2D<float4>   gSrcMip : register(t0);
RWTexture2D<float4> gDstMip : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dstCoord = dispatchThreadID.xy;

    if (dstCoord.x >= gDstSize.x || dstCoord.y >= gDstSize.y)
        return;

    uint2 srcCoord = dstCoord * 2;

    float4 c0 = gSrcMip[srcCoord + uint2(0, 0)];
    float4 c1 = gSrcMip[srcCoord + uint2(1, 0)];
    float4 c2 = gSrcMip[srcCoord + uint2(0, 1)];
    float4 c3 = gSrcMip[srcCoord + uint2(1, 1)];

    gDstMip[dstCoord] = (c0 + c1 + c2 + c3) * 0.25f;
}
