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

cbuffer TextureMetaCB : register(b1)
{
    float4 gMainTextureInfo;      // IsAttached, LerpToSecondary, UvTilingX, UvTilingY
    float4 gSecondaryTextureInfo;
    float4 gNormalTextureInfo;    // IsAttached, Strength, UvTilingX, UvTilingY
    float4 gSpecularTextureInfo;  // IsAttached, SpecInt, RoughMul, MetalMul
    float4 gHeightTextureInfo;

    float  gForceMipLevel;
    float  gUseForcedMip;
    float2 _PaddingMip;
};

cbuffer DirectionalLightCB : register(b2)
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

//~ Textures
Texture2D gMainTexture  : register(t0);
Texture2D gDisplacement : register(t1);
Texture2D gNormalMap    : register(t2);
Texture2D gSpecularMap  : register(t3);
Texture2D gHeightMap    : register(t4);

SamplerState gSampler : register(s0);

struct PSInput
{
    float4 Position  : SV_POSITION;
    float3 WorldPos  : TEXCOORD0;
    float3 Normal    : TEXCOORD1;
    float3 Tangent   : TEXCOORD2;
    float3 Bitangent : TEXCOORD3;
    float2 TexCoord  : TEXCOORD4;
    float3 Color     : COLOR0;
};

static float Has(float x)
{
    return step(0.5f, saturate(x));
}

float3 GetNormalWS(PSInput input)
{
    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    float hasNormal = Has(gNormalTextureInfo.x);
    float strength  = saturate(gNormalTextureInfo.y);

    float2 uv = input.TexCoord * gNormalTextureInfo.zw;

    float3 nTS;
    if (gUseForcedMip > 0.5f)
        nTS = gNormalMap.SampleLevel(gSampler, uv, gForceMipLevel).xyz;
    else
        nTS = gNormalMap.Sample(gSampler, uv).xyz;

    nTS = nTS * 2.0f - 1.0f;

    nTS = normalize(lerp(float3(0, 0, 1), nTS, hasNormal * strength));
    return normalize(mul(nTS, TBN));
}

float4 main(PSInput input) : SV_TARGET
{
    //~ Base color
    float hasBase = Has(gMainTextureInfo.x);
    float2 uv    = input.TexCoord * gMainTextureInfo.zw;

    float4 baseSample;
    if (gUseForcedMip > 0.5f)
        baseSample = gMainTexture.SampleLevel(gSampler, uv, gForceMipLevel);
    else
        baseSample = gMainTexture.Sample(gSampler, uv);

    float3 baseColor = lerp(input.Color, baseSample.rgb, hasBase);
    float  alpha     = lerp(1.0f,        baseSample.a,   hasBase);

    //~ Normal
    float3 N = GetNormalWS(input);

    //~ Lighting
    float3 L = normalize(-gDirLight_DirectionWS);
    float  NdotL = saturate(dot(N, L));

    float3 lightColor = gDirLight_Color * gDirLight_Intensity;

    float3 ambient = 0.05f * baseColor;
    float3 diffuse = baseColor * lightColor * NdotL;

    //~ Specular
    float3 V = normalize(gCameraPosition - input.WorldPos);
    float3 H = normalize(L + V);

    float hasSpec = Has(gSpecularTextureInfo.x);
    float specInt = gSpecularTextureInfo.y;
    float rough   = max(0.001f, gSpecularTextureInfo.z);

    float shininess = 2.0f / (rough * rough);
    float specTerm  = pow(saturate(dot(N, H)), shininess);

    float3 specSample;
    if (gUseForcedMip > 0.5f)
        specSample = gSpecularMap.SampleLevel(gSampler, uv, gForceMipLevel).rgb;
    else
        specSample = gSpecularMap.Sample(gSampler, uv).rgb;

    float3 specColor = lerp(float3(1,1,1), specSample, hasSpec);
    float3 specular  = specColor * specInt * specTerm * lightColor;

    float3 finalRgb = ambient + diffuse + specular;
    return float4(finalRgb, alpha);
}
