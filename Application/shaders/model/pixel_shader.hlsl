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

    // Individuals
    const float aoInd    = ApplyOcclusionTex(uv);
    const float roughInd = SampleRoughness(uv);
    const float metalInd = SampleMetallic(uv);

    // ORM values
    const float3 orm = SampleORM(uv);
    const float  aoOrm    = orm.x;
    const float  roughOrm = orm.y;
    const float  metalOrm = orm.z;

    // Flags
    const float hasOrm = step(0.5f, ORM_Meta.x);
    const float mixed  = step(0.5f, ORM_Meta.y); 

    const float ao    = lerp(aoInd,    lerp(aoOrm,    aoInd,    mixed), hasOrm);
    const float rough = lerp(roughInd, lerp(roughOrm, roughInd, mixed), hasOrm);
    const float metal = lerp(metalInd, lerp(metalOrm, metalInd, mixed), hasOrm);

    const float gloss = saturate(1.0f - rough);

    // Metallic workflow split (important!)
    const float3 diffuseColor = base * (1.0f - metal);

    const float3 dielectricF0 = float3(0.04f, 0.04f, 0.04f);
    const float3 F0           = lerp(dielectricF0, base, metal);

    const float3 specColor = SampleSpecularColor(uv, F0);

    // Convert spec color to a scalar strength (todo: make it config)
    const float specStrength = saturate(dot(specColor, float3(0.333333f, 0.333333f, 0.333333f)));
    const float glossFinal   = gloss * specStrength;

    float3 lit = 0.0f;
    lit += ComputeDirectionalLightsLambert(diffuseColor, N);
    lit += ComputePointLightsDiffuseSpec_BlinnPhong(diffuseColor, N, input.WorldPos, glossFinal);
    lit += ComputeSpotLightsDiffuseSpec_BlinnPhong(diffuseColor, N, input.WorldPos, glossFinal);

    // AO
    lit *= ao;

    // Emissive
    lit += ApplyEmissiveTex(uv);

    // Tonemap clamp
    lit = lit / (1.0f + lit);

    return float4(lit, 1.0f);
}
