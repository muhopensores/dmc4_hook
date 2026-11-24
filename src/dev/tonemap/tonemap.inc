// The tone map's pixel shader source (ToneMap.cpp includes it as one string). In the game, Write Source puts a copy in
// nativePC\shader\tonemap\tonemap.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// tonemap.hlsl: dmc4_hook ToneMap, in place of XfFilter.bfx's final composite pixel shaders (Vibed\ToneMap\README.md).
// TONE says which family the original is: 1 Reinhard with the exposure in gXfMiddleGray (a stage's tone map with manual or
// CPU exposure, 96 shaders), 2 Reinhard with the exposure read from the 1 x 1 adaptation texture PointSampler4 (auto
// exposure, 88), 0 no tone map at all (2 x scene + bloom, clipped at white: the stages with EnableToneMapping off, i.e.
// nearly all of them, 24). Each original is one combination of: the afterimage blend (PointSampler1: two forms), the
// scotopic blue shift, extended or plain Reinhard (gXfMaxLuminance), and any of three bloom layers (LinearSampler1-3). The
// mod compiles this once per original with its registers (REG_<name> / HAS_<name> from its constant table) and the
// features its code shows (TONE, SCOTOPIC, BLEND_ALPHA, EXT_TEX: TONE 2's extended Reinhard, its white in the adaptation
// texel's .y). With the curve at Stock it computes exactly what the original did
// (TONE 0: L, no curve):
//   colour = PointSampler0 (blended with PointSampler1 by how alike they are), L = colour x 2 (the scene buffer holds
//   colour / 2), blue shift toward luma x (1.05, 0.97, 1.27) by saturate(1 - (MG + 1.5) / 4.1), x = L x gXfMiddleGray (the
//   engine's exposure), out = x / (1 + x) or x (1 + x MaxLum) / (1 + x), plus the bloom layers x gXfScaleColor (.x layer
//   1, .z layer 2, .y layer 3), alpha 1.
// The other curves work in linear light: x is gamma-space (every DMC4 shader lights in gamma), so x^2.2 is the light,
// scaled by 2^EV, put through ACES (Narkowicz's fit) or AgX (Wrensch's minimal fit of Sobotka's AgX, base look), back to
// gamma. Each curve's input scale makes the stock's mid point (x = 1 -> 0.5 out) land on 0.5 too, so EV 0 keeps the
// game's brightness. Settings in c200 (the mod writes them when one of these shaders is bound; the originals use c0-c50).

#ifndef TM_STOCK_ONLY
#define TM_STOCK_ONLY 0         // 1: the stock maths only (the offline check against the originals)
#endif
#ifndef TONE
#define TONE 1
#endif
#ifndef SCOTOPIC
#define SCOTOPIC 0
#endif
#ifndef EXT_TEX
#define EXT_TEX 0
#endif
#ifndef BLEND_ALPHA
#define BLEND_ALPHA 0
#endif

#if TONE == 1
float gXfMiddleGray : register(REG_gXfMiddleGray);
#elif TONE == 2
sampler2D PointSampler4 : register(REG_PointSampler4);     // 1 x 1: the adapted exposure in .x
#endif
#ifdef HAS_gXfMaxLuminance
float gXfMaxLuminance : register(REG_gXfMaxLuminance);
#endif
#ifdef HAS_gXfScaleColor
float4 gXfScaleColor : register(REG_gXfScaleColor);
#endif
sampler2D PointSampler0 : register(REG_PointSampler0);
#ifdef HAS_PointSampler1
sampler2D PointSampler1 : register(REG_PointSampler1);
#endif
#ifdef HAS_LinearSampler1
sampler2D LinearSampler1 : register(REG_LinearSampler1);
#endif
#ifdef HAS_LinearSampler2
sampler2D LinearSampler2 : register(REG_LinearSampler2);
#endif
#ifdef HAS_LinearSampler3
sampler2D LinearSampler3 : register(REG_LinearSampler3);
#endif

// x curve (0 stock, 1 ACES, 2 AgX, 3 none: the stage-without-curve path, 2 x scene, for an image that is already
// display-ready, the RTX Remix bridge's), y exposure as a light factor (2^EV), z the same in gamma (2^(EV / 2.2)), w 1 = written
float4 gTmTune : register(c200);

// the curves' input scales: light 1 (the stock's x = 1) -> 0.5 out in gamma. Without a tone map (TONE 0) the stock's mid
// point is L = 0.5 -> 0.5, so the curves get x = 2 L.
#define ACES_MID 0.1513
#define AGX_MID 0.1833

struct PSIn {
    float4 uv : TEXCOORD0;      // xy this pixel; with the blend, zw the pixel the blend compares with
};

// Reinhard; extended (x (1 + x w) / (1 + x), w = 1 / white^2) when the original had a white
float3 curve_stock(float3 x, float w) {
#if defined(HAS_gXfMaxLuminance) || EXT_TEX
    return x * (1.0 + x * w) / (1.0 + x);
#else
    return x / (1.0 + x);
#endif
}

#if !TM_STOCK_ONLY
float3 curve_aces(float3 lin) {
    const float3 x = lin * ACES_MID;
    return pow(saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14)), 1.0 / 2.2);
}

float3 curve_agx(float3 lin) {
    const float3x3 to_agx = float3x3(0.842479062253094, 0.0423282422610123, 0.0423756549057051,
                                     0.0784335999999992, 0.878468636469772, 0.0784336,
                                     0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const float3x3 from_agx = float3x3(1.19687900512017, -0.0528968517574562, -0.0529716355144438,
                                       -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
                                       -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    const float lo = -12.47393, hi = 4.026069;
    float3 c = mul(lin * AGX_MID, to_agx);
    c = (clamp(log2(max(c, 1e-10)), lo, hi) - lo) / (hi - lo);
    const float3 c2 = c * c, c4 = c2 * c2;
    c = 15.5 * c4 * c2 - 40.14 * c4 * c + 31.96 * c4 - 6.868 * c2 * c + 0.4298 * c2 + 0.1191 * c - 0.00232;
    return saturate(mul(c, from_agx));          // display-encoded already (the fit's output is gamma 2.2)
}
#endif

float4 main(PSIn i) : COLOR0 {
#ifdef HAS_PointSampler1
    const float4 here = tex2D(PointSampler0, i.uv.zw);
    const float4 prev = tex2D(PointSampler1, i.uv.xy);
    const float d = dot(abs(prev.rgb - here.rgb), 1.0);
#if BLEND_ALPHA
    const float w = 0.5 * max(1.0 - 4.0 * d, prev.a);
#else
    const float w = max(0.5 - 2.0 * d, 0.0);
#endif
    // the RTX Remix bridge's image (curve 3): no afterimage blend. The previous output it blends with is at twice the
    // scene's scale, so on the handed-over image old frames hardly faded (characters trailed, 2026-11-24)
    const float w_used = (gTmTune.w > 0.5 && gTmTune.x > 2.5) ? 0.0 : w;
    const float3 colour = lerp(tex2D(PointSampler0, i.uv.xy).rgb, prev.rgb, w_used);
#else
    const float3 colour = tex2D(PointSampler0, i.uv.xy).rgb;
#endif
    float3 L = colour * 2.0;
#if TONE == 1
    const float exposure = gXfMiddleGray;
#elif TONE == 2
    const float4 adapted = tex2D(PointSampler4, float2(0.5, 0.5));
    const float exposure = adapted.x;
#else
    const float exposure = 2.0;                 // only for the curves (TONE 0 has no exposure of its own)
#endif
#if defined(HAS_gXfMaxLuminance)
    const float white = gXfMaxLuminance;
#elif TONE == 2 && EXT_TEX
    const float white = adapted.y;
#else
    const float white = 0.0;
#endif
#if SCOTOPIC && TONE != 0
    const float blue = saturate(1.0 - (exposure + 1.5) * 0.243902445);
    L += blue * (dot(L, float3(0.2125, 0.7154, 0.0721)) * float3(1.05, 0.97, 1.27) - L);
#endif
    const float3 x = L * exposure;

#if TM_STOCK_ONLY
#if TONE == 0
    float3 o = L;
#else
    float3 o = curve_stock(x, white);
#endif
#else
    const bool tuned = gTmTune.w > 0.5;
    const float pick = tuned ? gTmTune.x : 0.0;
    float3 o;
    if (pick < 0.5) {
#if TONE == 0
        o = L * (tuned ? gTmTune.z : 1.0);
#else
        o = curve_stock(x * (tuned ? gTmTune.z : 1.0), white);
#endif
    } else if (pick > 2.5) {
        o = colour * 2.0;
    } else {
        const float3 lin = pow(max(x, 0.0), 2.2) * gTmTune.y;
        o = pick < 1.5 ? curve_aces(lin) : curve_agx(lin);
    }
#endif

#ifdef HAS_LinearSampler1
    o += tex2D(LinearSampler1, i.uv.xy).rgb * gXfScaleColor.x;
#endif
#ifdef HAS_LinearSampler3
    o += tex2D(LinearSampler3, i.uv.xy).rgb * gXfScaleColor.y;
#endif
#ifdef HAS_LinearSampler2
    o += tex2D(LinearSampler2, i.uv.xy).rgb * gXfScaleColor.z;
#endif
    return float4(o, 1.0);
}
)hlsl"
