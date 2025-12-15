Texture2D gSceneColor : register(t0);
SamplerState gSamp0   : register(s0);

cbuffer PostCB : register(b0)
{
    float Exposure;
    float Invert;
    float Time;
    float _Pad;
};

struct PSIn
{
    float4 PositionCS : SV_POSITION;
    float2 UV         : TEXCOORD0;
};

float4 main(PSIn input) : SV_TARGET
{
    float3 col = gSceneColor.Sample(gSamp0, input.UV).rgb;

    col *= max(Exposure, 0.0f);
    col = lerp(col, 1.0f - col, saturate(Invert));

    return float4(col, 1.0f);
}
