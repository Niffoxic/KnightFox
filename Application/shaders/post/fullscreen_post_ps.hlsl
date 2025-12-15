Texture2D gSceneColor : register(t0);
SamplerState gSamp0   : register(s0);

cbuffer PostCB : register(b0)
{
    // Basic grading
    float Exposure;
    float Gamma;
    float Contrast;
    float Saturation;

    float Invert;
    float Grayscale;
    float Time;
    float Fade;

    // Resolution
    float2 Resolution;
    float2 InvResolution;

    // Vignette and blur and sharpen
    float Vignette;
    float VignettePower;
    float BlurStrength;
    float SharpenStrength;

    float GrainStrength;
    float ChromAbStrength;
    float ScanlineStrength;
    float DitherStrength;

    // Mouse
    float2 MousePosPixels;
    float2 MousePosUV;

    float3 MouseButtons;    // L, R, M 
    float  MouseWheel;

    // Color grading
    float Temperature; 
    float Tint;
    float HueShift;
    float TonemapType;

    float WhitePoint;
    float BloomStrength;
    float BloomThreshold;
    float BloomKnee;

    float LensDistortion;
    float Letterbox;
    float LetterboxSoftness;
    float RadialBlurStrength;

    float RadialBlurRadius;
    float3 _Pad0;
};

struct PSIn
{
    float4 PositionCS : SV_POSITION;
    float2 UV         : TEXCOORD0;
};

float3 ApplyContrast(float3 c, float contrast)
{
    return (c - 0.5f) * contrast + 0.5f;
}

float3 ApplySaturation(float3 c, float sat)
{
    float luma = dot(c, float3(0.299f, 0.587f, 0.114f));
    return lerp(luma.xxx, c, sat);
}

float2 Rotate2D(float2 v, float a)
{
    float s = sin(a), c = cos(a);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float3 RGBToYIQ(float3 c)
{
    //~ hue rotation space
    return float3(
        dot(c, float3(0.299f, 0.587f, 0.114f)),
        dot(c, float3(0.596f,-0.274f,-0.322f)),
        dot(c, float3(0.211f,-0.523f, 0.312f))
    );
}

float3 YIQToRGB(float3 yiq)
{
    return float3(
        dot(yiq, float3(1.0f, 0.956f, 0.621f)),
        dot(yiq, float3(1.0f,-0.272f,-0.647f)),
        dot(yiq, float3(1.0f,-1.105f, 1.702f))
    );
}

float3 ApplyHueShift(float3 c, float hueShift)
{
    float3 yiq = RGBToYIQ(c);
    float2 iq  = Rotate2D(yiq.yz, hueShift * 6.2831853f); //~ hueShift
    yiq.yz = iq;
    return YIQToRGB(yiq);
}

float3 ApplyTempTint(float3 c, float temperature, float tint)
{
    float3 t = float3(temperature, tint, -temperature);
    return c + 0.10f * t;
}

float3 Tonemap_Reinhard(float3 x) { return x / (1.0f + x); }

// very lightweight curve
float3 Tonemap_ACES(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 Tonemap_Filmic(float3 x)
{
    x = max(0.0f, x - 0.004f);
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

// Deterministic noise
float Hash12(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 34.345f);
    return frac(p.x * p.y);
}

// Soft threshold for bloom extraction
float SoftKnee(float x, float threshold, float knee)
{
    float k = max(knee, 1e-5f);
    float t = threshold;
    float lo = t - k;
    float hi = t + k;
    float a = saturate((x - lo) / (hi - lo));
    return (x - t) * a;
}

float3 SampleScene(float2 uv)
{
    return gSceneColor.Sample(gSamp0, uv).rgb;
}

float3 ApplyBloomCheap(float2 uv, float3 baseCol)
{
    // 5 taps around pixel then bloom is extracted for brightness only
    float3 c0 = SampleScene(uv);

    float2 o1 = float2(1, 0) * InvResolution;
    float2 o2 = float2(0, 1) * InvResolution;

    float3 c1 = SampleScene(uv + o1);
    float3 c2 = SampleScene(uv - o1);
    float3 c3 = SampleScene(uv + o2);
    float3 c4 = SampleScene(uv - o2);

    float3 avg = (c0 + c1 + c2 + c3 + c4) * 0.2f;

    float lum = dot(avg, float3(0.299f, 0.587f, 0.114f));

    //~ soft threshold
    float bloom = SoftKnee(lum, BloomThreshold, BloomKnee);
    bloom = max(bloom, 0.0f);

    return baseCol + avg * (bloom * BloomStrength);
}

float3 ApplySharpen(float2 uv, float3 col)
{
    //~ center - neighbors
    float2 o1 = float2(1, 0) * InvResolution;
    float2 o2 = float2(0, 1) * InvResolution;

    float3 c  = SampleScene(uv);
    float3 n  = SampleScene(uv + o2);
    float3 s  = SampleScene(uv - o2);
    float3 e  = SampleScene(uv + o1);
    float3 w  = SampleScene(uv - o1);

    float3 blur = (n + s + e + w) * 0.25f;
    float3 high = c - blur;
    return col + high * SharpenStrength;
}

float3 ApplyBlurCheap(float2 uv, float3 col)
{
    // 9 taps
    float2 px = InvResolution;

    float3 a = SampleScene(uv);
    float3 b = SampleScene(uv + float2( px.x, 0));
    float3 c = SampleScene(uv + float2(-px.x, 0));
    float3 d = SampleScene(uv + float2(0,  px.y));
    float3 e = SampleScene(uv + float2(0, -px.y));
    float3 f = SampleScene(uv + float2( px.x,  px.y));
    float3 g = SampleScene(uv + float2(-px.x,  px.y));
    float3 h = SampleScene(uv + float2( px.x, -px.y));
    float3 i = SampleScene(uv + float2(-px.x, -px.y));

    float3 blur = (a + b + c + d + e + f + g + h + i) / 9.0f;
    return lerp(col, blur, saturate(BlurStrength));
}

float2 ApplyLensDistortion(float2 uv)
{
    // Barrel and pincushion around center (thanks yt)
    float2 p = uv * 2.0f - 1.0f;
    float r2 = dot(p, p);
    float k  = LensDistortion;
    p *= (1.0f + k * r2);
    return p * 0.5f + 0.5f;
}

float3 ApplyChromaticAberration(float2 uv)
{
    // Shift R and B in opposite directions from the center
    float2 dir = (uv - 0.5f);
    float2 off = dir * (ChromAbStrength * 2.0f) * InvResolution * 200.0f; //~ scalable effect

    float3 cR = SampleScene(uv + off);
    float3 cG = SampleScene(uv);
    float3 cB = SampleScene(uv - off);

    return float3(cR.r, cG.g, cB.b);
}

float3 ApplyRadialBlur(float2 uv, float3 baseCol)
{
    float strength = RadialBlurStrength;

    //~ Make left click boost radial blur 
    strength *= (1.0f + MouseButtons.x * 0.75f);

    strength = max(strength, 0.0f);
    if (strength <= 0.0001f)
        return baseCol;

    float2 center = MousePosUV; //~ interactive center
    float2 dir = uv - center;

    float dist = length(dir);
    float r = max(RadialBlurRadius, 1e-5f);

    //~ iff only inside radius
    float mask = 1.0f - saturate(dist / r);
    float w = strength * mask;

    //~ 6 samples towards center
    float3 acc = baseCol;
    float2 stepv = -dir * (w * 0.15f);

    acc += SampleScene(uv + stepv * 1.0f);
    acc += SampleScene(uv + stepv * 2.0f);
    acc += SampleScene(uv + stepv * 3.0f);
    acc += SampleScene(uv + stepv * 4.0f);
    acc += SampleScene(uv + stepv * 5.0f);

    acc *= (1.0f / 6.0f);
    return lerp(baseCol, acc, saturate(w));
}

float3 ApplyScanlines(float2 uv, float3 col)
{
    float s = max(ScanlineStrength, 0.0f);
    if (s <= 0.0001f)
        return col;

    //~ lines per screen height
    float v = sin((uv.y * Resolution.y) * 3.14159f + Time * 10.0f);
    float scan = 1.0f - s * (0.5f + 0.5f * v) * 0.15f;
    return col * scan;
}

float3 ApplyGrain(float2 uv, float3 col)
{
    float g = saturate(GrainStrength);
    if (g <= 0.0001f)
        return col;

    float n = Hash12(uv * Resolution + Time * 60.0f) - 0.5f;
    return col + n.xxx * (0.08f * g);
}

float3 ApplyDither(float2 uv, float3 col)
{
    float d = saturate(DitherStrength);
    if (d <= 0.0001f)
        return col;

    // dithering hash (thanks youtube!)
    float n = Hash12(floor(uv * Resolution)) - 0.5f;
    return col + n.xxx * (InvResolution.x * 2.0f) * d;
}

float3 ApplyVignette(float2 uv, float3 col)
{
    float v = saturate(Vignette);
    if (v <= 0.0001f)
        return col;

    float2 d = abs(uv - 0.5f) * 2.0f;
    float vig = pow(1.0f - saturate(dot(d, d)), max(VignettePower, 0.001f));
    return col * lerp(1.0f, vig, v);
}

float3 ApplyInvertGray(float3 col)
{
    col = lerp(col, 1.0f - col, saturate(Invert));

    float g = dot(col, float3(0.299f, 0.587f, 0.114f));
    col = lerp(col, g.xxx, saturate(Grayscale));
    return col;
}

float3 ApplyTonemap(float3 col)
{
    float wp = max(WhitePoint, 0.001f);
    col /= wp;

    uint tm = (uint)TonemapType;
    if (tm == 0u) return col;
    if (tm == 1u) return Tonemap_Reinhard(col);
    if (tm == 2u) return Tonemap_ACES(col);
    return Tonemap_Filmic(col);
}

float2 ApplyLetterboxUV(float2 uv, out float letterMask)
{
    // 1 means visible, 0 means black bars
    float a = saturate(Letterbox);
    if (a <= 0.0001f)
    {
        letterMask = 1.0f;
        return uv;
    }

    float softness = max(LetterboxSoftness, 1e-5f);
    float bar = a * 0.5f; // TODO: make it configurable later: thickness split top and bottom

    float top    = smoothstep(0.0f, softness, uv.y - bar);
    float bottom = smoothstep(0.0f, softness, (1.0f - bar) - uv.y);
    letterMask = top * bottom;
    return uv;
}

float4 ApplyPostEffects(float2 inUV)
{
    float2 uv = inUV;

    // Letterbox mask so to not change uv but masking
    float letterMask = 1.0f;

    uv = ApplyLetterboxUV(uv, letterMask);
    uv = ApplyLensDistortion(uv);

    // Clamp UV so sampling so that it can never goes the not define
    uv = saturate(uv);

    // chromatic effect 
    float3 col = ApplyChromaticAberration(uv);

    // Exposure
    col *= max(Exposure, 0.0f);

    col = ApplyBlurCheap(uv, col);
    col = ApplySharpen(uv, col);
    col = ApplyBloomCheap(uv, col);
    col = ApplyContrast(col, Contrast);
    col = ApplySaturation(col, Saturation);
    col = ApplyTempTint(col, Temperature, Tint);
    col = ApplyHueShift(col, HueShift);
    col = ApplyInvertGray(col);
    col = ApplyVignette(uv, col);
    col = ApplyRadialBlur(uv, col);

    col = ApplyScanlines(uv, col);
    col = ApplyGrain(uv, col);
    col = ApplyDither(uv, col);
    col = ApplyTonemap(col);
    col = pow(max(col, 0.0f), 1.0f / max(Gamma, 0.001f)); //~ gamma

    //~ Fade
    col = lerp(col, 0.0f.xxx, saturate(Fade));

    //~ Apply letterbox mask at the end 
    col *= letterMask;

    return float4(col, 1.0f);
}

float4 main(PSIn input) : SV_TARGET
{
    return ApplyPostEffects(input.UV);
}
