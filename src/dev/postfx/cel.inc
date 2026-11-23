// The cel shading pixel shader's source (Cel.cpp includes it as one string). In the game, Write file puts a copy in
// nativePC\shader\postfx\cel.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// cel.hlsl: dmc4_hook PostFX cel shading, the pixel shader of XfMaterialStandard's skinned SPECULAR_POWMAP permutations
// (Vibed\PostFX\README.md, step 4). With cel shading on, the skinned lit keys (characters) are pointed at these records,
// so only characters draw with it. The vertex shader stays the key's own, and the engine uploads what the record lists:
//   - the inputs are the stock vertex shader's outputs, by semantic (as material_pbr.hlsl);
//   - every parameter's register comes from the replaced shader's constant table: the mod defines REG_<name> and
//     HAS_<name> for each one it lists, and nothing else may be read;
//   - the permutation comes in as LIGHTING (1 4SPOT, 2 SH4SPOT, 3 EMITSH4SPOT, 4 THINSH4SPOT), NORMALMAP (0 / 1), LIGHTMAP
//     (0 NONE, 5 VCOLOR).
// Lighting in the stock shaders' gamma space: light = band(N.L) per light instead of N.L (lit or shaded, a soft edge),
// ambient flattened toward the scene's average, a hard-edged highlight.

#ifndef NORMALMAP
#define NORMALMAP 1
#endif
#ifndef LIGHTMAP
#define LIGHTMAP 0
#endif
#ifndef LIGHTING
#define LIGHTING 2
#endif

// ---- tuning (the Graphics tab's sliders write c210 / c211 when one of these shaders is bound; defaults until then) ----
float4 gCelTune0 : register(c210);     // band edge (N.L), edge softness, ambient flatness, highlight strength
float4 gCelTune1 : register(c211);     // shaded side's share of the light, highlight size, ambient on the shaded side, 1 = written
#define TUNE0 (gCelTune1.w > 0.5 ? gCelTune0 : float4(0.25, 0.04, 0.7, 0.6))
#define TUNE1 (gCelTune1.w > 0.5 ? gCelTune1.xyz : float3(0.3, 0.5, 0.6))
#define K_EDGE      (TUNE0.x)
#define K_SOFT      (TUNE0.y)
#define K_FLAT      (TUNE0.z)
#define K_HIGHLIGHT (TUNE0.w)
#define K_SHADED    (TUNE1.x)
#define K_HL_SIZE   (TUNE1.y)
#define K_AMB_SHADE (TUNE1.z)

// ---- parameters (registers from the replaced shader) ----
#ifdef HAS_gXfLightEnable0
bool gXfLightEnable0 : register(REG_gXfLightEnable0);
#endif
#ifdef HAS_gXfLightEnable1
bool gXfLightEnable1 : register(REG_gXfLightEnable1);
#endif
#ifdef HAS_gXfLightEnable2
bool gXfLightEnable2 : register(REG_gXfLightEnable2);
#endif
#ifdef HAS_gXfLightEnable3
bool gXfLightEnable3 : register(REG_gXfLightEnable3);
#endif
#ifdef HAS_gXfSimpleLightEnable0
bool gXfSimpleLightEnable0 : register(REG_gXfSimpleLightEnable0);
#endif
#ifdef HAS_gXfSimpleLightEnable1
bool gXfSimpleLightEnable1 : register(REG_gXfSimpleLightEnable1);
#endif
#ifdef HAS_gXfSimpleLightEnable2
bool gXfSimpleLightEnable2 : register(REG_gXfSimpleLightEnable2);
#endif
#ifdef HAS_gXfSimpleLightEnable3
bool gXfSimpleLightEnable3 : register(REG_gXfSimpleLightEnable3);
#endif
#ifdef HAS_gXfLightParam
float4 gXfLightParam[16] : register(REG_gXfLightParam);
#endif
#ifdef HAS_gXfSHc
float4 gXfSHc[7] : register(REG_gXfSHc);
#endif
#ifdef HAS_gXfLightSimple
float4 gXfLightSimple[8] : register(REG_gXfLightSimple);
#endif
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
#ifdef HAS_gXfBaseMapScale
float4 gXfBaseMapScale : register(REG_gXfBaseMapScale);
#endif
#ifdef HAS_gXfBlendFactor
float gXfBlendFactor : register(REG_gXfBlendFactor);
#endif
#ifdef HAS_gXfDetailFactor
float4 gXfDetailFactor : register(REG_gXfDetailFactor);
#endif
#ifdef HAS_gXfFresnelFactor
float4 gXfFresnelFactor : register(REG_gXfFresnelFactor);
#endif
#ifdef HAS_gXfLightMapScale
float3 gXfLightMapScale : register(REG_gXfLightMapScale);
#endif
#ifdef HAS_XfSamplerAlbedoMap
sampler2D XfSamplerAlbedoMap : register(REG_XfSamplerAlbedoMap);
#endif
#ifdef HAS_XfSamplerNormalMap
sampler2D XfSamplerNormalMap : register(REG_XfSamplerNormalMap);
#endif
#ifdef HAS_XfSamplerMaskMap
sampler2D XfSamplerMaskMap : register(REG_XfSamplerMaskMap);
#endif
#ifdef HAS_XfSamplerAdditionalMap
sampler2D XfSamplerAdditionalMap : register(REG_XfSamplerAdditionalMap);
#endif

// ---- inputs: the stock vertex shader's outputs (TEXCOORD3 starts the tangent frame, or the normal alone) ----
struct PSIn {
    float4 uv0  : TEXCOORD0;    // xy albedo UV, z blend factor (output alpha), w direct-light occlusion (tangent.w)
    float3 wpos : TEXCOORD1;    // world position
    float2 uv2  : TEXCOORD2;    // normal / mask / additional map UV
#if LIGHTMAP == 5
    float3 vcol : COLOR0;       // vertex colour: rgb ambient, r specular occlusion
#endif
#if NORMALMAP == 1
    float3 tan  : TEXCOORD3;
    float3 bin  : TEXCOORD4;
    float3 nrm  : TEXCOORD5;
#else
    float3 nrm  : TEXCOORD3;
#endif
};

// lit (1) or shaded (K_SHADED), with a soft edge at K_EDGE
float band(float nl) {
    return lerp(K_SHADED, 1.0, smoothstep(K_EDGE - K_SOFT, K_EDGE + K_SOFT, nl));
}

// one light: banded diffuse, and a hard highlight where (N.H)^power passes the size threshold
void add_light(inout float3 diff, inout float3 spec, float3 N, float3 V, float3 L, float3 radiance, float dfac, float power) {
    const float nl = dot(N, L);
    diff += radiance * band(nl) * dfac;
    const float nh = saturate(dot(N, normalize(L + V)));
    const float h = pow(nh, power);
    spec += radiance * smoothstep(K_HL_SIZE - 0.02, K_HL_SIZE + 0.02, h) * step(0.0, nl);
}

#ifdef HAS_gXfSHc
float3 sh(float3 n) {
    const float4 n4 = float4(n, 1.0);
    const float4 q = n.yzzx * n.xyzz;
    const float3 a = float3(dot(gXfSHc[0], n4), dot(gXfSHc[1], n4), dot(gXfSHc[2], n4));
    const float3 b = float3(dot(gXfSHc[3], q), dot(gXfSHc[4], q), dot(gXfSHc[5], q));
    return max(a + b + gXfSHc[6].rgb * (n.x * n.x - n.y * n.y), 0.0);
}
// the ambient light's average over all directions (the SH's constant term)
float3 sh_average() { return max(float3(gXfSHc[0].w, gXfSHc[1].w, gXfSHc[2].w), 0.0); }
// where most of the ambient light comes from (its first-order terms, by luminance): the key light of a scene lit mostly by
// its surroundings, as DMC4's are
float3 sh_key() {
    const float3 d = 0.2126 * gXfSHc[0].xyz + 0.7152 * gXfSHc[1].xyz + 0.0722 * gXfSHc[2].xyz;
    return dot(d, d) > 1e-8 ? normalize(d) : float3(0, 1, 0);
}
#endif

float4 main(PSIn i) : COLOR0 {
    // ---- the material (as the stock shader) ----
    const float4 base = tex2D(XfSamplerAlbedoMap, i.uv0.xy);
#ifdef HAS_gXfBaseMapScale
    const float4 bms = gXfBaseMapScale;
#else
    const float4 bms = float4(1, 1, 1, 0);
#endif
    const float3 albedo = base.rgb * max(base.a, bms.w) * bms.xyz;
    const float alpha = max(base.a, 1.0 - bms.w) * i.uv0.z;
    const float tw = i.uv0.w;
#ifdef HAS_XfSamplerMaskMap
    const float3 mask = tex2D(XfSamplerMaskMap, i.uv2).rgb;
#else
    const float3 mask = 0.0;
#endif
#ifdef HAS_gXfFresnelFactor
    const float power = max(gXfFresnelFactor.z, 1.0);
#else
    const float power = 32.0;
#endif

#if NORMALMAP == 1
    const float4 nm = tex2D(XfSamplerNormalMap, i.uv2);
    const float3 t = float3(nm.r, nm.g, nm.a) * 2.0 - float3(1.0352941, 1.0196078, 1.0);
    const float nx = t.z * t.x, ny = t.y;
    const float nz = sqrt(saturate(1.0 - nx * nx - ny * ny));
    const float3 N = normalize(nx * i.tan + ny * i.bin + nz * i.nrm);
#else
    const float3 N = normalize(i.nrm);
#endif
#ifdef HAS_gXfEyePos
    const float3 eye_to = i.wpos - gXfEyePos;
#else
    const float3 eye_to = float3(0, 0, 1);
#endif
    const float dist = length(eye_to);
    const float3 V = -eye_to / dist;

    // ---- direct light, banded ----
    float3 diff = 0, spec = 0;
#ifdef HAS_gXfLightParam
    [unroll] for (int k = 0; k < 4; ++k) {
        bool on = k == 0 ? gXfLightEnable0 : k == 1 ? gXfLightEnable1 : k == 2 ? gXfLightEnable2 : gXfLightEnable3;
        if (on) {
            const float4 p0 = gXfLightParam[4 * k], p1 = gXfLightParam[4 * k + 1], p2 = gXfLightParam[4 * k + 2],
                         p3 = gXfLightParam[4 * k + 3];
            const float3 lv = p1.xyz - i.wpos;
            const float d2 = dot(lv, lv);
            const float inv = rsqrt(d2);
            const float3 L = lv * inv;
            const float fall = 1.0 - saturate((d2 * inv - p0.w) * p1.w);
            const float cone = min(max(p3.w, (dot(-p2.xyz, L) - p3.x) * p3.y), 1.0);
            add_light(diff, spec, N, V, L, p0.rgb * (fall * cone), p3.z, power);
        }
    }
#endif
#ifdef HAS_gXfLightSimple
    [unroll] for (int j = 0; j < 4; ++j) {
        bool on = j == 0 ? gXfSimpleLightEnable0 : j == 1 ? gXfSimpleLightEnable1 : j == 2 ? gXfSimpleLightEnable2
                                                                                          : gXfSimpleLightEnable3;
        if (on) {
            const float4 p0 = gXfLightSimple[2 * j], p1 = gXfLightSimple[2 * j + 1];
            const float3 lv = p1.xyz - i.wpos;
            const float d2 = dot(lv, lv);
            const float inv = rsqrt(d2);
            float3 s = 0;
            add_light(diff, s, N, V, lv * inv, p0.rgb * (1.0 - saturate((d2 * inv - p0.w) * p1.w)), 1.0, power);
        }
    }
#endif
    diff *= tw;
    spec *= tw * mask * K_HIGHLIGHT;

    // ---- ambient, flattened toward its average ----
    float3 amb = 0;
#ifdef HAS_gXfSHc
    // flattened toward the ambient light from the key direction (the brightest): toward the average instead darkened every
    // surface facing the sky, which gets well above the average (room 400, 2026-11-23: "it might be too dark")
    const float3 key = sh_key();
    amb = lerp(sh(N), sh(key), K_FLAT);
    // two tones from the ambient light too: lit toward its key direction, K_AMB_SHADE of it on the other side
    amb *= lerp(K_AMB_SHADE, 1.0, smoothstep(K_EDGE - K_SOFT, K_EDGE + K_SOFT, dot(N, key)));
#endif
#ifdef HAS_gXfDetailFactor
    const float ao_bias = gXfDetailFactor.w;
#else
    const float ao_bias = 0.5;
#endif
#ifdef HAS_gXfLightMapScale
    const float3 lms = gXfLightMapScale;
#else
    const float3 lms = 1.0;
#endif
#if LIGHTMAP == 5
    amb += i.vcol * lms * (1.0 + ao_bias * (tw - 1.0));
    spec *= i.vcol.r;
#endif

    float3 emit = 0;
#if LIGHTING == 3
#ifdef HAS_XfSamplerAdditionalMap
    emit = tex2D(XfSamplerAdditionalMap, i.uv2).r * lms;   // EMITSH4SPOT: emission, times the albedo
#endif
#endif

    float3 col = albedo * (diff + amb + emit) + spec;

    // ---- out: fog, the "LDR x 2" encoding ----
#if defined(HAS_gXfFogFactor) && defined(HAS_gXfFogPlane) && defined(HAS_gXfFogColor) && defined(HAS_gXfFogEnable)
    const float f1 = (dist - gXfFogFactor.x) * gXfFogFactor.z;
    const float f2 = dot(gXfFogPlane, float4(i.wpos, 1.0)) * gXfFogFactor.w;
    float fog = max(min(max(f1, f2), gXfFogColor.w), 0.0);
#ifdef HAS_gXfBlendFactor
    fog *= gXfBlendFactor;
#endif
    fog = gXfFogEnable > 0.0 ? fog : 0.0;
    col = lerp(col, gXfFogColor.rgb, fog);
#endif
#ifdef HAS_gXfLDR2Offset
    const float ofs = gXfLDR2Offset;
#else
    const float ofs = 0.0;
#endif
    return float4(col * 0.5 + ofs, alpha);
}
)hlsl"
