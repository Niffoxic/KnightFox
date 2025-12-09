cbuffer Test : register(b0)
{
    float  iTime;
    float  padding0;
    float2 iResolution;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.position.xy / iResolution;

    float pulse = 0.5 + 0.5 * sin(iTime * 2.0);

    float3 finalColor = input.color * pulse;

    return float4(finalColor, 1.0f);
}
