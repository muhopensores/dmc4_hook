// The soft shadow receiver's pixel shader source (ShadowReceiver.cpp includes it as one string). Edit it here; keep each
// raw string under 16 KB (MSVC's limit for one literal). In the game, Write Source puts a copy in
// nativePC\shader\shadows\shadow_receiver.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// shadow_receiver.hlsl: dmc4_hook Shadows, the pixel shader of tXfMaterialShadowReceiver's cascade permutations.
// Plan and evidence: Vibed\Shadows\README.md (step 2). The stock vertex shader and the engine's uploads stay, so:
//   - the inputs below are the stock vertex shader's outputs, by semantic;
//   - every parameter's register comes from the replaced shader's constant table (REG_<name> / HAS_<name>, as in
//     PbrMaterials), and the features follow from which parameters it has: XfSamplerAlbedoMap (alpha-tested casters'
//     alpha), XfSamplerShadowMap + XfShadowMaskPow (the baked shadow mask), XfShadowBorderGradation (FUNC_SHADOW_CASTER:
//     faces turning from the light darken gradually instead of being skipped), XfSamplerShadowBlocker* (blocker maps).
// What the stock shader does (XfMaterialShadowReceiver.bfx, pixel shaders 0-95 / 179-210, mode 0 = 3 cascades): skip the
// pixel past XfShadowMapRange.z or (not CASTER) facing away from the light; pick the cascade by view depth (Near below
// range.x, Middle below range.y, else Far, which fades out by XfShadowMapRangeGrad); compare depth taps of the R32F map
// (shadowed where the pixel's depth >= the stored one; the bias is in the cast pass) on a 4 x 4 grid of whole texels
// blended into a bilinear 3 x 3 filter (XfShadowPCFRadius.y = map size); output rgb = XfShadowColor (fogged) x 0.5 +
// gXfLDR2Offset, alpha = shadow x XfShadowColor.w, blended over the lit frame.
//
// Here the filter is contact-hardening (PCSS): a blocker search finds how far in front of the pixel the occluders are, the
// penumbra grows with that gap (the light's size, LIGHT_SIZE cm per cm), and a rotated disc of bilinear depth compares
// filters it. The cascade matrices come from the mod (hooked where the game hands them to the receiver pass) so the gap
// and the radius are in centimetres whatever the cascade: the softness doesn't jump at a cascade border. Without them
// (an older build, or before the first frame) the filter is the disc at MIN_RADIUS texels, about the stock softness.
//
// Settings (defines, the mod sets them): SHADOW_TAPS (filter taps), SEARCH_TAPS (blocker search taps), SHADOW_DEBUG (0
// off, 1 cascades in colour, 2 the penumbra radius: green 1 texel .. red MAX), and the live values in c210 / c211.

#ifndef SHADOW_TAPS
#define SHADOW_TAPS 24
#endif
#ifndef SEARCH_TAPS
#define SEARCH_TAPS 16
#endif
#ifndef SHADOW_DEBUG
#define SHADOW_DEBUG 0
#endif
// LIGHT_SIZE: penumbra width per cm between the occluder and the pixel (the light's apparent size; the sun is ~0.01).
// MAX_SOFT: the widest penumbra in cm (also how far the blocker search looks). MIN_RADIUS: the narrowest filter, texels.
#ifndef LIGHT_SIZE
#define LIGHT_SIZE 0.03
#endif
#ifndef MAX_SOFT
#define MAX_SOFT 40.0
#endif
#ifndef MIN_RADIUS
#define MIN_RADIUS 1.0
#endif

// The mod's registers (no stock receiver shader uses a float register above c13).
float4 gShTune : register(c210);    // LIGHT_SIZE, MAX_SOFT, MIN_RADIUS, 1 = written by the mod
float4 gShFlags : register(c211);   // x 1 = the cascade matrices below are current
float4 gShMat[12] : register(c212); // cascade c (near, middle, far): rows 4c..4c+3 give u, v, z, w = dot(float4(world, 1), row)

#define K_LIGHT_SIZE (gShTune.w > 0.5 ? gShTune.x : LIGHT_SIZE)
#define K_MAX_SOFT   (gShTune.w > 0.5 ? gShTune.y : MAX_SOFT)
#define K_MIN_RADIUS (gShTune.w > 0.5 ? gShTune.z : MIN_RADIUS)

#ifdef HAS_gXfEyePos
float3 gXfEyePos : register(REG_gXfEyePos);
#endif
#ifdef HAS_gXfLDR2Offset
float gXfLDR2Offset : register(REG_gXfLDR2Offset);
#endif
#ifdef HAS_gXfFogEnable
float gXfFogEnable : register(REG_gXfFogEnable);
#endif
#ifdef HAS_gXfFogFactor
float4 gXfFogFactor : register(REG_gXfFogFactor);
#endif
#ifdef HAS_gXfFogPlane
float4 gXfFogPlane : register(REG_gXfFogPlane);
#endif
#ifdef HAS_gXfFogColor
float4 gXfFogColor : register(REG_gXfFogColor);
#endif
float4 XfShadowColor : register(REG_XfShadowColor);
float3 XfShadowLightDir : register(REG_XfShadowLightDir);
float4 XfShadowPCFRadius : register(REG_XfShadowPCFRadius);
float3 XfShadowMapRange : register(REG_XfShadowMapRange);
float4 XfShadowMapRangeGrad : register(REG_XfShadowMapRangeGrad);
#ifdef HAS_XfShadowBorderGradation
float XfShadowBorderGradation : register(REG_XfShadowBorderGradation);
#endif
#ifdef HAS_XfShadowMaskPow
float XfShadowMaskPow : register(REG_XfShadowMaskPow);
#endif
#ifdef HAS_XfSamplerAlbedoMap
sampler2D XfSamplerAlbedoMap : register(REG_XfSamplerAlbedoMap);
#endif
#ifdef HAS_XfSamplerShadowMap
sampler2D XfSamplerShadowMap : register(REG_XfSamplerShadowMap);
#endif
sampler2D XfSamplerShadowNear : register(REG_XfSamplerShadowNear);
sampler2D XfSamplerShadowMiddle : register(REG_XfSamplerShadowMiddle);
sampler2D XfSamplerShadowFar : register(REG_XfSamplerShadowFar);
#ifdef HAS_XfSamplerShadowBlockerNear
#define BLOCKER 1
sampler2D XfSamplerShadowBlockerNear : register(REG_XfSamplerShadowBlockerNear);
sampler2D XfSamplerShadowBlockerMiddle : register(REG_XfSamplerShadowBlockerMiddle);
sampler2D XfSamplerShadowBlockerFar : register(REG_XfSamplerShadowBlockerFar);
#else
#define BLOCKER 0
#endif

// The stock vertex shader's outputs. TEXCOORD5 is the albedo UV when the albedo map is there, else the mask UV; with both,
// the mask UV is TEXCOORD6.
struct PSIn {
    float4 wpos : TEXCOORD0;        // xyz world position (cm), w view depth
    float3 nrm  : TEXCOORD1;        // world normal
    float4 sc0  : TEXCOORD2;        // the near / middle / far cascade's projected position (divide by w)
    float4 sc1  : TEXCOORD3;
    float4 sc2  : TEXCOORD4;
#if defined(HAS_XfSamplerAlbedoMap) || defined(HAS_XfSamplerShadowMap)
    float2 uv5  : TEXCOORD5;
#endif
#if defined(HAS_XfSamplerAlbedoMap) && defined(HAS_XfSamplerShadowMap)
    float2 uv6  : TEXCOORD6;
#endif
    float2 vpos : VPOS;
};

// cascade c's map position (uv, depth) of a world point
float3 project(int c, float3 p) {
    const float4 P = float4(p, 1.0);
    const float w = dot(P, gShMat[4 * c + 3]);
    return float3(dot(P, gShMat[4 * c]), dot(P, gShMat[4 * c + 1]), dot(P, gShMat[4 * c + 2])) / w;
}

// Map units per cm here: uv across the light, depth toward it (by differences, so a perspective map works too)
void scales(int c, float3 p, float3 L, out float uv_cm, out float z_cm) {
    const float s = 10.0;
    const float3 a = project(c, p);
    const float3 t1 = normalize(cross(L, abs(L.y) < 0.9 ? float3(0, 1, 0) : float3(1, 0, 0)));
    const float3 t2 = cross(L, t1);
    const float3 b = project(c, p + t1 * s);
    const float3 d = project(c, p + t2 * s);
    const float3 e = project(c, p - L * s);    // toward the light (-L)
    uv_cm = 0.5 * (length(b.xy - a.xy) + length(d.xy - a.xy)) / s;
    z_cm = max(abs(a.z - e.z) / s, 1e-12);
}

// Tap k of n on a Vogel disc (even coverage for any n), turned by the pixel's angle
float2 disc(float k, float n, float turn) {
    float2 sc;
    sincos(k * 2.3999632 + turn, sc.x, sc.y);
    return sqrt((k + 0.5) / n) * sc.yx;
}

// The stock rule per depth tap: shadowed where the pixel is at or behind the stored depth. With blocker maps, a tap in
// front of the blocker depth doesn't count for a pixel behind it (stock 179-210).
float4 compare(float4 d, float z, float bz) {
    float4 s = step(d, z);
#if BLOCKER
    if (z > bz) s *= step(bz, d);
#endif
    return s;
}

// Bilinear depth compare at uv (4 whole texels, weighted like the stock filter)
float pcf(sampler2D sm, float2 uv, float z, float size, float bz) {
    const float2 t = uv * size - 0.5;
    const float2 f = frac(t);
    const float2 c = (t - f + 0.5) / size;
    const float e = 1.0 / size;
    float4 d;
    d.x = tex2Dlod(sm, float4(c, 0, 0)).r;
    d.y = tex2Dlod(sm, float4(c + float2(e, 0), 0, 0)).r;
    d.z = tex2Dlod(sm, float4(c + float2(0, e), 0, 0)).r;
    d.w = tex2Dlod(sm, float4(c + float2(e, e), 0, 0)).r;
    const float4 s = compare(d, z, bz);
    return lerp(lerp(s.x, s.y, f.x), lerp(s.z, s.w, f.x), f.y);
}
)hlsl"
R"hlsl(
// One cascade's shadow (0 lit .. 1 shadowed); radius_texels: the filter radius used (debug view)
float cascade(int c, sampler2D sm, float4 sc, float3 p, float3 L, float turn, out float radius_texels
#if BLOCKER
              , sampler2D bm
#endif
              ) {
    const float3 q = sc.xyz / sc.w;
    const float size = XfShadowPCFRadius.y;
    const float texel = 1.0 / size;
#if BLOCKER
    const float bz = tex2Dlod(bm, float4(q.xy, 0, 0)).r;
#else
    const float bz = 0;
#endif
    float radius = K_MIN_RADIUS * texel;
    if (gShFlags.x > 0.5) {
        float uv_cm, z_cm;
        scales(c, p, L, uv_cm, z_cm);
        // the blocker search covers the widest penumbra (at most 48 texels: the far cascade's cm are many texels short)
        const float wide = clamp(K_MAX_SOFT * uv_cm, radius, 48.0 * texel);
        float sum = 0, count = 0;
        [loop] for (int k = 0; k < SEARCH_TAPS; ++k) {
            const float d = tex2Dlod(sm, float4(q.xy + disc(k, SEARCH_TAPS, turn) * wide, 0, 0)).r;
            float hit = d < q.z ? 1.0 : 0.0;
#if BLOCKER
            if (q.z > bz && d < bz) hit = 0;
#endif
            sum += d * hit;
            count += hit;
        }
        if (count == 0) {           // nothing in front of the pixel anywhere near: lit
            radius_texels = 0;
            return 0;
        }
        const float gap_cm = max(q.z - sum / count, 0) / z_cm;
        radius = clamp(gap_cm * K_LIGHT_SIZE * uv_cm, radius, wide);
    }
    float s = 0;
    [loop] for (int k = 0; k < SHADOW_TAPS; ++k)
        s += pcf(sm, q.xy + disc(k, SHADOW_TAPS, turn) * radius, q.z, size, bz);
    radius_texels = radius * size;
    return s / SHADOW_TAPS;
}

#if BLOCKER
#define CASCADE(c, sm, bm, sc) cascade(c, sm, sc, i.wpos.xyz, L, turn, radius, bm)
#else
#define CASCADE(c, sm, bm, sc) cascade(c, sm, sc, i.wpos.xyz, L, turn, radius)
#endif

float4 main(PSIn i) : COLOR0 {
    clip(XfShadowMapRange.z - i.wpos.w);
    const float3 L = XfShadowLightDir;
    const float ndl = dot(normalize(i.nrm), -L);
#ifndef HAS_XfShadowBorderGradation
    clip(ndl);
#endif
    // the disc's turn per pixel (interleaved gradient noise): the leftover pattern is a fine, even grain
    const float turn = 6.2831853 * frac(52.9829189 * frac(dot(i.vpos, float2(0.06711056, 0.00583715))));
    const float depth = i.wpos.w;
    float shadow, radius;
    float alpha = XfShadowColor.w;
    int which;
    if (depth < XfShadowMapRange.x) {
        shadow = CASCADE(0, XfSamplerShadowNear, XfSamplerShadowBlockerNear, i.sc0);
        which = 0;
    } else if (depth < XfShadowMapRange.y) {
        shadow = CASCADE(1, XfSamplerShadowMiddle, XfSamplerShadowBlockerMiddle, i.sc1);
        which = 1;
    } else {
        shadow = CASCADE(2, XfSamplerShadowFar, XfSamplerShadowBlockerFar, i.sc2);
        alpha *= 1.0 - saturate(depth * XfShadowMapRangeGrad.x + XfShadowMapRangeGrad.y);
        which = 2;
    }
#ifdef HAS_XfShadowBorderGradation
    shadow = max(shadow, 1.0 - saturate(ndl * XfShadowBorderGradation));
#endif
    float a = shadow * alpha;
#ifdef HAS_XfSamplerAlbedoMap
    a *= tex2D(XfSamplerAlbedoMap, i.uv5).a;
#endif
#ifdef HAS_XfSamplerShadowMap
#ifdef HAS_XfSamplerAlbedoMap
    a *= pow(saturate(tex2D(XfSamplerShadowMap, i.uv6).r), XfShadowMaskPow);
#else
    a *= pow(saturate(tex2D(XfSamplerShadowMap, i.uv5).r), XfShadowMaskPow);
#endif
#endif

    float3 col = XfShadowColor.rgb;
#if defined(HAS_gXfFogEnable) && defined(HAS_gXfFogFactor) && defined(HAS_gXfFogPlane) && defined(HAS_gXfFogColor) && defined(HAS_gXfEyePos)
    const float fd = (length(gXfEyePos - i.wpos.xyz) - gXfFogFactor.x) * gXfFogFactor.z;
    const float fh = dot(gXfFogPlane, float4(i.wpos.xyz, 1.0)) * gXfFogFactor.w;
    const float f = max(min(gXfFogColor.w, max(fd, fh)), 0.0);
    if (gXfFogEnable > 0) col = lerp(col, gXfFogColor.rgb, f);
#endif
#ifdef HAS_gXfLDR2Offset
    const float ldr = gXfLDR2Offset;
#else
    const float ldr = 0;
#endif

#if SHADOW_DEBUG == 1
    // cascades: near red, middle green, far blue, over every receiver
    const float3 tint = which == 0 ? float3(1, 0.2, 0.2) : (which == 1 ? float3(0.2, 1, 0.2) : float3(0.2, 0.4, 1));
    return float4(tint * 0.5 + ldr, 0.35 + 0.4 * shadow);
#elif SHADOW_DEBUG == 2
    // the filter radius where something shadows: green = 1 texel (sharp) .. red = 16 texels and up
    const float r = saturate((radius - 1.0) / 15.0);
    return float4(float3(r, 1.0 - r, 0.1) * 0.5 + ldr, radius > 0 ? 0.6 : 0.0);
#else
    return float4(col * 0.5 + ldr, a);
#endif
}
)hlsl"
