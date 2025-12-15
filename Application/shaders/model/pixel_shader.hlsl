#include "../common/common_pixel.hlsl"

struct PSInput
{
    float4 PositionCS   : SV_POSITION;

    float3 WorldPos     : TEXCOORD0;
    float3 WorldNormal  : TEXCOORD1;
    float3 WorldTangent : TEXCOORD2;
    float3 WorldBitan   : TEXCOORD3;

    float2 TexCoord0    : TEXCOORD4;
    float2 TexCoord1    : TEXCOORD5;
};

float4 main(PSInput input) : SV_TARGET
{
    const float2 uv = ApplyDisplacementUV(input.TexCoord0, input.WorldPos);

    float3 base = float3(1.0f, 1.0f, 1.0f);
    base = ApplyBaseColorTex(uv, base);

    float3 N = ApplyNormalTex(
        uv,
        input.WorldNormal,
        input.WorldTangent,
        input.WorldBitan
    );

    float3 lit = ApplyFakeLighting(base, N, input.WorldPos);

    const float ao = ApplyOcclusionTex(uv);
    lit *= ao;

    const float gloss = SampleGloss(uv);
    lit = ApplyFakeSpecular(lit, N, input.WorldPos, gloss);

    return float4(lit, 1.0f);
}
