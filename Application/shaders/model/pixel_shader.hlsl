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
    ApplyOpacityCutout(input.TexCoord0);
    float2 uv = input.TexCoord0;

    //~ Height parallax and displacement
    uv = ApplyHeightParallaxUV(uv, input.WorldPos);
    uv = ApplyDisplacementUV(uv, input.WorldPos);

    //~ BaseColor
    float3 base = float3(1.0f, 1.0f, 1.0f);
    base = ApplyBaseColorTex(uv, base);

    //~ Normals
    float3 N = ApplyNormalTex(
        uv,
        input.WorldNormal,
        input.WorldTangent,
        input.WorldBitan
    );
    N = ApplyDetailNormalTex(uv, N, input.WorldTangent, input.WorldBitan);

    //~ Material reads
    const float gloss = SampleGloss(uv);

    //~ Light accumulation
    float3 lit = 0.0f;
    lit += ComputeDirectionalLightsLambert(base, N);
    lit += ComputePointLightsDiffuseSpec_BlinnPhong(base, N, input.WorldPos, gloss);
    lit += ComputeSpotLightsDiffuseSpec_BlinnPhong(base, N, input.WorldPos, gloss);

    //~ AO
    const float ao = ApplyOcclusionTex(uv);
    lit *= ao;

    //~ Emissive
    lit += ApplyEmissiveTex(uv);

    //~ tonemap clamp
    lit = lit / (1.0f + lit);

    return float4(lit, 1.0f);
}
