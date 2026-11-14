// The PBR pixel shader's source (PbrMaterials.cpp includes it as one string). Edit it here; it's split into raw
// strings under 16 KB each (MSVC's limit for one literal), joined by the compiler. In the game, Write Source puts a copy
// in nativePC\shader\pbr\material_pbr.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// material_pbr.hlsl: dmc4_hook PbrMaterials, the pixel shader of XfMaterialStandard's lit SPECULAR_RIM permutations.
// Plan and evidence: Vibed\PBR\README.md. The stock vertex shader and the engine's parameter uploads stay, so:
//   - the inputs below are the stock vertex shader's outputs, by semantic;
//   - every parameter's register comes from the replaced shader's constant table: the mod defines REG_<name> (b0, c16,
//     s2, ...) and HAS_<name> for each one it lists, and nothing else may be read;
//   - the permutation comes in as LIGHTING (1 4SPOT, 2 SH4SPOT, 3 EMITSH4SPOT, 4 THINSH4SPOT), NORMALMAP (0 / 1), LIGHTMAP
//     (0 NONE, 1 STANDARD, 2 SHADOW, 5 VCOLOR).
// Settings: PBR_SHADOW (1: the stage shadow is shaded here, see "the stage shadow" below; the Shadows mod binds its maps),
// PBR_LINEAR (light in linear space, the scene stays gamma), PBR_SIMPLE_SPEC (the 4 point lights get specular),
// PBR_REFLECT (what reflects: 0 the scene's ambient light, 1 the cube relit to it, 2 the cube as is; see below), PBR_DEBUG
// (0 off, 1 albedo, 2 normal, 3 roughness, 4 metal, 5 occlusion, 6 diffuse light, 7 specular).
//
// The material (Albam's PBR mode):
//   MaskMap = ORM: R occlusion, G roughness (perceptual), B metal
//   gXfFresnelFactor = (roughness scale, metal scale, dielectric F0, reflection strength): the record's FresnelFactor,
//     FresnelBias, SpecularPow and EnvMapPower (x mColor2.w)
//   gXfEnvMapFactor = the environment cube's last mip (envmap_bias 5): roughness 1 reads it
// Subsurface scattering = lighting THINSH4SPOT (LIGHTING 4), the game's own "light through thin surfaces" permutation:
//   gXfLightMapScale (the record's lightmap_factor, the stock THIN colour) = the scatter colour: its largest channel is how
//     far light spreads past the shadow line (0 none .. 1 far), its hue (half desaturated) the colour of that light, and
//     it is the colour of light coming through. Skin ~ (0.5, 0.2, 0.12).
//   AdditionalMap R = thickness (0 thin: ears, fingers, nostrils, light comes through .. 1 thick: none), G = where it
//     scatters (0 = plain PBR: nails, eyebrows). No texture = the engine's NullWhite: scatters everywhere, transmits nowhere.

#ifndef PBR_LINEAR
#define PBR_LINEAR 1
#endif
// LIGHT_GAMMA: with Linear lighting on, the exponent textures and light colours are converted with on the way in (and back
// on the way out). 2.2 = true linear lighting; 1.0 = the game's own gamma-space lighting; in between trades the cleaner
// light falloff of linear for a closer match with the vanilla models and stages around (the user, 2026-11-14: linear
// looks better, gamma fits the surroundings better).
#ifndef LIGHT_GAMMA
#define LIGHT_GAMMA 2.2
#endif
#ifndef PBR_SIMPLE_SPEC
#define PBR_SIMPLE_SPEC 1
#endif
#ifndef PBR_DEBUG
#define PBR_DEBUG 0
#endif
#ifndef PBR_REFLECT
#define PBR_REFLECT 0
#endif
#ifndef PBR_SHADOW
#define PBR_SHADOW 1
#endif
#ifndef PBR_CUBE_CONE
#define PBR_CUBE_CONE 0         // 1: rough reflections averaged over a cone of a mid mip (see cube_radiance)
#endif
#ifndef PBR_SHADOW_TAPS
#define PBR_SHADOW_TAPS 16
#endif

// ---- subsurface scattering tuning (defaults; the PbrMaterials panel's Tuning sliders override them live) ----
// SSS_WIDTH: scales how far light reaches past the shadow line (1 = the material's scatter colour as authored).
// SSS_SATURATION: how much of the scatter colour's hue the extra light takes (0 = neutral, 1 = the full colour).
// SSS_STRENGTH: scales the extra light itself (0 = no scattering, plain PBR).
#ifndef SSS_WIDTH
#define SSS_WIDTH 0.7
#endif
#ifndef SSS_SATURATION
#define SSS_SATURATION 0.37
#endif
#ifndef SSS_STRENGTH
#define SSS_STRENGTH 1.0
#endif

// ---- reflection tuning ----
// REFLECT_STRENGTH: scales every PBR material's reflection of its surroundings (light highlights aren't affected). The
// stock character shaders reflect nothing (EnvMapPower 0), so in dim places, where nearly all light is ambient, PBR's
// reflection of that ambient made dark materials look glossy next to vanilla models.
// VERTEX_AO: how much the per-vertex occlusion (tangent.w, Albam's mtfw_tangent_w; the stock shaders darken direct light
// by it) also occludes the ambient light and its reflection (0 = direct light only, as the stock shaders).
#ifndef REFLECT_STRENGTH
#define REFLECT_STRENGTH 1.0
#endif
#ifndef VERTEX_AO
#define VERTEX_AO 1.0
#endif
// TRANSMISSION: scales the light coming through thin parts from behind (THINSH4SPOT, AdditionalMap R < 1).
#ifndef TRANSMISSION
#define TRANSMISSION 1.1
#endif

// ---- live tuning ----
// The PbrMaterials panel's Tuning sliders write the values above into c200 / c201 every frame, with c201.w = 1 (no game
// pixel shader uses a float register above c61, and the engine uploads each parameter to its own register only, so
// nothing else touches them). Until it has (a dmc4_hook without the sliders, the first frame after a device reset) the
// defines above apply.
float4 gPbrTune0 : register(c200);     // SSS width, SSS saturation, SSS strength, reflection strength
float4 gPbrTune1 : register(c201);     // vertex occlusion, light gamma, transmission, 1 = written by the panel
#define TUNE0 (gPbrTune1.w > 0.5 ? gPbrTune0 : float4(SSS_WIDTH, SSS_SATURATION, SSS_STRENGTH, REFLECT_STRENGTH))
#define TUNE1 (gPbrTune1.w > 0.5 ? gPbrTune1.xyz : float3(VERTEX_AO, LIGHT_GAMMA, TRANSMISSION))
#define K_SSS_WIDTH        (TUNE0.x)
#define K_SSS_SATURATION   (TUNE0.y)
#define K_SSS_STRENGTH     (TUNE0.z)
#define K_REFLECT_STRENGTH (TUNE0.w)
#define K_VERTEX_AO        (TUNE1.x)
#define K_LIGHT_GAMMA      (TUNE1.y)
#define K_TRANSMISSION     (TUNE1.z)
#ifndef NORMALMAP
#define NORMALMAP 1
#endif
#ifndef LIGHTMAP
#define LIGHTMAP 0
#endif
#ifndef LIGHTING
#define LIGHTING 2
#endif

#define PI 3.14159265

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
#ifdef HAS_gXfEnvMapFactor
float gXfEnvMapFactor : register(REG_gXfEnvMapFactor);
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
#ifdef HAS_XfSamplerEnvironmentMap
samplerCUBE XfSamplerEnvironmentMap : register(REG_XfSamplerEnvironmentMap);
#endif
#ifdef HAS_XfSamplerLightMap
sampler2D XfSamplerLightMap : register(REG_XfSamplerLightMap);
#endif
#ifdef HAS_XfSamplerShadowMap
sampler2D XfSamplerShadowMap : register(REG_XfSamplerShadowMap);
#endif
#ifdef HAS_XfSamplerAdditionalMap
sampler2D XfSamplerAdditionalMap : register(REG_XfSamplerAdditionalMap);
#endif

// ---- inputs: the stock vertex shader's outputs (Vibed\PBR\README.md: TEXCOORD3 is the lightmap UV when there is one,
// then the tangent frame, or the normal alone) ----
#if LIGHTMAP == 1 || LIGHTMAP == 2
#define TC_N0 TEXCOORD4
#define TC_N1 TEXCOORD5
#define TC_N2 TEXCOORD6
#else
#define TC_N0 TEXCOORD3
#define TC_N1 TEXCOORD4
#define TC_N2 TEXCOORD5
#endif

struct PSIn {
    float4 uv0  : TEXCOORD0;    // xy albedo UV, z blend factor (output alpha), w direct-light occlusion (tangent.w)
    float3 wpos : TEXCOORD1;    // world position
    float2 uv2  : TEXCOORD2;    // normal / mask / additional map UV
#if LIGHTMAP == 1 || LIGHTMAP == 2
    float2 uv3  : TEXCOORD3;    // lightmap / shadow map UV
#endif
#if LIGHTMAP == 5
    float3 vcol : COLOR0;       // vertex colour: rgb ambient, r specular occlusion
#endif
#if NORMALMAP == 1
    float3 tan  : TC_N0;
    float3 bin  : TC_N1;
    float3 nrm  : TC_N2;
#else
    float3 nrm  : TC_N0;
#endif
#if PBR_SHADOW
    float2 vpos : VPOS;         // the pixel's screen position: the shadow filter's per-pixel turn
#endif
};

// ---- helpers ----
float3 to_lin(float3 c) {
#if PBR_LINEAR
    return pow(max(c, 0.0), K_LIGHT_GAMMA);
#else
    return c;
#endif
}

float3 to_out(float3 c) {
#if PBR_LINEAR
    return pow(max(c, 0.0), 1.0 / K_LIGHT_GAMMA);
#else
    return c;
#endif
}

struct Surf {
    float3 N, V, f0;
    float  nv, a, a2;
    float3 Ng;          // the vertex normal (no normal map)
    float3 Nb;          // the normal seen by scattered light: the normal map's detail blurred toward Ng
    float  wrap;        // how far scattered light reaches past the shadow line (0 = Lambert)
    float3 tint;        // the colour of that extra light (the scatter colour, half desaturated)
};

// One light: diffuse (before albedo, times 1 - F) and specular into the sums. Lights are in the stock shader's units, where a
// white Lambert surface shows the light's colour x N.L (no 1 / pi), so the specular BRDF is scaled by pi to match.
void add_light(inout float3 diff, inout float3 spec, Surf s, float3 L, float3 radiance, float dfac, bool with_spec) {
    const float nl = saturate(dot(s.N, L));
    const float3 H = normalize(L + s.V);
    const float nh = saturate(dot(s.N, H));
    const float vh = saturate(dot(s.V, H));
    const float3 F = s.f0 + (1.0 - s.f0) * pow(1.0 - vh, 5.0);
#if LIGHTING == 4
    // Subsurface scattering, the cheap form of pre-integrated skin: Lambert as for any material, plus the light that
    // scatters past the shadow line (wrapped Lambert on a blurred normal) where it exceeds Lambert, in the scatter tint.
    // The first version wrapped and blurred each channel by its own width, so red was lit apart from green and blue on
    // every shadow edge and normal-map bump, and linear lighting turned that into pink skin (2B, 2026-11-14).
    // Energy-conserving wrap, / (1 + w)^2: below Lambert on the lit side, so the extra light is only a band around the
    // shadow line (N.L < w / ((1 + w)^2 - 1), 0.4 for w = 0.5). The plain / (1 + w) form added up to a third of the light
    // over the whole lit side, and pale skin under a strong light clipped to pink (2B, 2026-11-14).
    const float lam = saturate(dot(s.N, L));
    const float scat = saturate((dot(s.Nb, L) + s.wrap) / ((1.0 + s.wrap) * (1.0 + s.wrap)));
    diff += radiance * (lam + max(scat - lam, 0.0) * K_SSS_STRENGTH * s.tint) * dfac * (1.0 - F);
#else
    diff += radiance * (nl * dfac) * (1.0 - F);
#endif
    if (with_spec) {
        const float d = nh * nh * (s.a2 - 1.0) + 1.0;
        const float D = s.a2 / (PI * d * d);
        const float Vis = 0.5 / (nl * (s.nv * (1.0 - s.a) + s.a) + s.nv * (nl * (1.0 - s.a) + s.a) + 1e-5);
        spec += radiance * (nl * D * Vis * PI) * F;
    }
}
)hlsl"
R"hlsl(
#if LIGHTING == 4
// Light coming through from behind (DICE's fast translucency, Barre-Brisebois & Bouchard 2011): strongest where the light
// shines straight through toward the eye, plus a little from any light behind the surface. Times thinness and the scatter
// colour in main().
float transmit(Surf s, float3 L) {
    const float3 Lt = normalize(L + s.Ng * 0.3);
    return pow(saturate(dot(s.V, -Lt)), 6.0) * 1.5 + saturate(-dot(s.Ng, L)) * 0.3;
}
#endif

#ifdef HAS_gXfLightParam
// Spot light i (gXfLightParam[4i..4i+3]), as the stock shader attenuates it.
void spot(inout float3 diff, inout float3 spec, inout float3 back, Surf s, float3 wpos, int i) {
    const float4 p0 = gXfLightParam[4 * i], p1 = gXfLightParam[4 * i + 1], p2 = gXfLightParam[4 * i + 2], p3 = gXfLightParam[4 * i + 3];
    const float3 lv = p1.xyz - wpos;
    const float d2 = dot(lv, lv);
    const float inv = rsqrt(d2);
    const float3 L = lv * inv;
    const float fall = 1.0 - saturate((d2 * inv - p0.w) * p1.w);
    const float cone = min(max(p3.w, (dot(-p2.xyz, L) - p3.x) * p3.y), 1.0);
    const float3 radiance = to_lin(p0.rgb) * (fall * cone);
    add_light(diff, spec, s, L, radiance, p3.z, true);
#if LIGHTING == 4
    back += radiance * transmit(s, L);
#endif
}
#endif

#ifdef HAS_gXfLightSimple
// Point light i (gXfLightSimple[2i..2i+1]): diffuse only in the stock shader.
void simple(inout float3 diff, inout float3 spec, inout float3 back, Surf s, float3 wpos, int i) {
    const float4 p0 = gXfLightSimple[2 * i], p1 = gXfLightSimple[2 * i + 1];
    const float3 lv = p1.xyz - wpos;
    const float d2 = dot(lv, lv);
    const float inv = rsqrt(d2);
    const float3 L = lv * inv;
    const float3 radiance = to_lin(p0.rgb) * (1.0 - saturate((d2 * inv - p0.w) * p1.w));
    add_light(diff, spec, s, L, radiance, 1.0, PBR_SIMPLE_SPEC != 0);
#if LIGHTING == 4
    back += radiance * transmit(s, L);
#endif
}
#endif

#ifdef HAS_gXfSHc
float3 sh(float3 n) {
    const float4 n4 = float4(n, 1.0);
    const float4 q = n.yzzx * n.xyzz;
    const float3 a = float3(dot(gXfSHc[0], n4), dot(gXfSHc[1], n4), dot(gXfSHc[2], n4));
    const float3 b = float3(dot(gXfSHc[3], q), dot(gXfSHc[4], q), dot(gXfSHc[5], q));
    return max(a + b + gXfSHc[6].rgb * (n.x * n.x - n.y * n.y), 0.0);
}
#endif

// Debug views of one value: a dim colour ramp (0 dark blue, 0.25 blue, 0.5 cyan-green, 0.75 yellow, 1 dark red), not grey. The game's
// auto exposure and bloom scale grey: on a dark stage 0.75 came out white and glowing, and close values looked the same.
float3 ramp(float v) {
    v = saturate(v);
    const float3 c = saturate(1.5 - abs(4.0 * v - float3(3.0, 2.0, 1.0)));   // "jet": dark blue, blue, cyan, green, yellow, red
    return c * 0.18;
}

// Specular occlusion from ambient occlusion (Lagarde & de Rousiers 2014): occluded places reflect less of the
// surroundings, rough surfaces more so.
float spec_occlusion(float nv, float occlusion, float rough) {
    return saturate(pow(nv + occlusion, exp2(-16.0 * rough - 1.0)) - 1.0 + occlusion);
}

// Split-sum environment BRDF, Karis' analytic fit (no lookup texture).
float3 env_brdf(float3 f0, float rough, float nv) {
    const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
    const float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
    const float4 r = rough * c0 + c1;
    const float a004 = min(r.x * r.x, exp2(-9.28 * nv)) * r.x + r.y;
    const float2 ab = float2(-1.04, 1.04) * a004 + r.zw;
    return f0 * ab.x + ab.y;
}

#ifdef HAS_XfSamplerEnvironmentMap
// A cube texel: rgb gamma, alpha a linear HDR scale (rgb / a). Converting the decoded value instead turned DefaultCube_CM's
// hot spots (8 decoded) into 97 and lit the silhouettes in sparkles.
float3 cube_decode(float4 c) { return to_lin(c.rgb) / max(c.a, 1.0 / 255.0); }

// The cube's light from direction R for a roughness. Its mips blur it, but only down to 16 x 16 per face: D3D9 has no
// seamless cube filtering and the game's mips are box filtered, so the 1 x 1 .. 4 x 4 mips drew each face as a flat patch
// with hard edges on rough spheres (the PBR test spheres, 2026-11-14), and a cone of taps on the 8 x 8 mip still showed
// the texels' edges ("football patches": the engine samples the cube with point filtering; PbrMaterials' Smooth cube
// filtering makes it linear). Past that mip the blur is 16 taps of the 16 x 16 mip on a spiral (Vogel) disc around R,
// weighted toward the centre and wider with the roughness (half-angle up to ~68 deg), so it crosses the faces' edges
// smoothly. lod_range = the cube's last mip (gXfEnvMapFactor with envmap_bias 5).
float3 cube_radiance(float3 R, float rough, float lod_range) {
#if !PBR_CUBE_CONE
    // the plain way (the default since 2026-11-14, a friend of the user's: "just turn on filtering"): the mip for the
    // roughness, filtered by the sampler (PbrMaterials' Smooth cube filtering makes it trilinear)
    return cube_decode(texCUBElod(XfSamplerEnvironmentMap, float4(R, rough * lod_range)));
#endif
    const float cap = max(lod_range - 4.0, 0.0);            // the 16 x 16 mip
    const float lod = rough * lod_range;
    if (lod <= cap) return cube_decode(texCUBElod(XfSamplerEnvironmentMap, float4(R, lod)));
    const float spread = 2.5 * saturate((lod - cap) / max(lod_range - cap, 1e-3));   // tan of the cone's half-angle
    const float3 t1 = normalize(cross(R, abs(R.y) < 0.99 ? float3(0, 1, 0) : float3(1, 0, 0)));
    const float3 t2 = cross(R, t1);
    float3 sum = 0;
    float weight = 0;
    [loop] for (int k = 0; k < 16; ++k) {
        const float r = sqrt((k + 0.5) / 16.0);
        float2 sc;
        sincos(k * 2.3999632, sc.x, sc.y);
        const float w = 1.0 - 0.6 * r * r;                  // more of the centre, as a lobe has
        const float3 d = normalize(R + (t1 * sc.y + t2 * sc.x) * (r * spread));
        sum += w * cube_decode(texCUBElod(XfSamplerEnvironmentMap, float4(d, cap)));
        weight += w;
    }
    return sum / weight;
}
#endif

)hlsl"
R"hlsl(
// ---- the stage shadow (dmc4_hook Shadows step 3, Vibed\Shadows\README.md) ----
// The game draws the stage shadow as a pass of its own (tXfMaterialShadowReceiver) that pulls finished pixels toward a flat
// colour, so highlights and scattered light stayed visible inside shadows. With the Shadows mod's "Shadows in PBR
// materials" that pass skips these materials (hooks at its two material gates, 0x9F33B6 / 0x5287C2) and the shadow is
// shaded here: the same three cascade maps (s13-s15, bound by the mod, point sampled), the mod's contact-hardening filter,
// the stock CASTER receiver's darkening (toward the shadow colour by its darkness; faces turning from the shadow's light
// darken by XfShadowBorderGradation), and no direct highlights or light through thin parts where it darkens.
#if PBR_SHADOW
// [0] light size (penumbra cm per cm of gap), widest penumbra (cm), sharpest edge (texels), 1
// [1] x 1 = the values below are current, y the maps' size, z border gradation (0 = none), w darkness
// [2] XfShadowMapRange: where the near, middle, far cascades end (cm), [3] XfShadowMapRangeGrad (the far fade)
// [4] the shadow colour (XfShadowColorCaster), [5] the shadow's light direction (the way it travels)
// [6 + 4c .. 9 + 4c] cascade c's u, v, depth, w rows: dot(float4(world, 1), row)
float4 gPbrShadow[18] : register(c202);
sampler2D gPbrShadowNear : register(s13);
sampler2D gPbrShadowMiddle : register(s14);
sampler2D gPbrShadowFar : register(s15);

float3 shd_project(int c, float3 p) {
    const float4 P = float4(p, 1.0);
    const float w = dot(P, gPbrShadow[9 + 4 * c]);
    return float3(dot(P, gPbrShadow[6 + 4 * c]), dot(P, gPbrShadow[7 + 4 * c]), dot(P, gPbrShadow[8 + 4 * c])) / w;
}

// map units per cm at p: uv across the light, depth toward it (by differences)
void shd_scales(int c, float3 p, float3 L, out float uv_cm, out float z_cm) {
    const float s = 10.0;
    const float3 a = shd_project(c, p);
    const float3 t1 = normalize(cross(L, abs(L.y) < 0.9 ? float3(0, 1, 0) : float3(1, 0, 0)));
    const float3 t2 = cross(L, t1);
    const float3 b = shd_project(c, p + t1 * s);
    const float3 d = shd_project(c, p + t2 * s);
    const float3 e = shd_project(c, p - L * s);
    uv_cm = 0.5 * (length(b.xy - a.xy) + length(d.xy - a.xy)) / s;
    z_cm = max(abs(a.z - e.z) / s, 1e-12);
}

// tap k of n on a Vogel disc, turned by the pixel's angle
float2 shd_disc(float k, float n, float turn) {
    float2 sc;
    sincos(k * 2.3999632 + turn, sc.x, sc.y);
    return sqrt((k + 0.5) / n) * sc.yx;
}

// bilinear depth compare: shadowed where the pixel is at or behind the stored depth (the bias is in the cast pass)
float shd_pcf(sampler2D sm, float2 uv, float z, float size) {
    const float2 t = uv * size - 0.5;
    const float2 f = frac(t);
    const float2 c = (t - f + 0.5) / size;
    const float e = 1.0 / size;
    float4 d;
    d.x = tex2Dlod(sm, float4(c, 0, 0)).r;
    d.y = tex2Dlod(sm, float4(c + float2(e, 0), 0, 0)).r;
    d.z = tex2Dlod(sm, float4(c + float2(0, e), 0, 0)).r;
    d.w = tex2Dlod(sm, float4(c + float2(e, e), 0, 0)).r;
    const float4 s = step(d, z);
    return lerp(lerp(s.x, s.y, f.x), lerp(s.z, s.w, f.x), f.y);
}

// cascade c's shadow at p (0 lit .. 1 shadowed): blocker search, penumbra from the gap, a disc of bilinear compares
float shd_cascade(int c, sampler2D sm, float3 p, float3 L, float turn, float min_texels) {
    const float3 q = shd_project(c, p);
    const float size = gPbrShadow[1].y;
    const float texel = 1.0 / size;
    float uv_cm, z_cm;
    shd_scales(c, p, L, uv_cm, z_cm);
    const float narrow = min_texels * texel;
    const float wide = clamp(gPbrShadow[0].y * uv_cm, narrow, 48.0 * texel);
    float sum = 0, count = 0;
    [loop] for (int k = 0; k < 16; ++k) {
        const float d = tex2Dlod(sm, float4(q.xy + shd_disc(k, 16, turn) * wide, 0, 0)).r;
        const float hit = d < q.z ? 1.0 : 0.0;
        sum += d * hit;
        count += hit;
    }
    if (count == 0) return 0;
    const float gap_cm = max(q.z - sum / count, 0) / z_cm;
    const float radius = clamp(gap_cm * gPbrShadow[0].x * uv_cm, narrow, wide);
    float s = 0;
    [loop] for (int j = 0; j < PBR_SHADOW_TAPS; ++j)
        s += shd_pcf(sm, q.xy + shd_disc(j, PBR_SHADOW_TAPS, turn) * radius, q.z, size);
    return s / PBR_SHADOW_TAPS;
}
#endif
)hlsl"
R"hlsl(
float4 main(PSIn i) : COLOR0 {
    // ---- the material ----
    const float4 base = tex2D(XfSamplerAlbedoMap, i.uv0.xy);
#ifdef HAS_gXfBaseMapScale
    const float4 bms = gXfBaseMapScale;
#else
    const float4 bms = float4(1, 1, 1, 0);
#endif
    const float3 albedo = to_lin(base.rgb * max(base.a, bms.w) * bms.xyz);
    const float alpha = max(base.a, 1.0 - bms.w) * i.uv0.z;
    const float tw = i.uv0.w;

#ifdef HAS_XfSamplerMaskMap
    const float3 orm = tex2D(XfSamplerMaskMap, i.uv2).rgb;
#else
    const float3 orm = float3(1.0, 0.5, 0.0);
#endif
#ifdef HAS_gXfFresnelFactor
    const float4 knobs = gXfFresnelFactor;
#else
    const float4 knobs = float4(1.0, 1.0, 0.04, 1.0);
#endif
    const float rough = clamp(orm.g * knobs.x, 0.03, 1.0);
    const float metal = saturate(orm.b * knobs.y);
    // ambient occlusion: the ORM map's R and the per-vertex occlusion (tangent.w)
    const float ao = orm.r * lerp(1.0, saturate(i.uv0.w), K_VERTEX_AO);

    // the normal, decoded as the stock shader does (DXT5nm: x = (2a - 1)(2r - 1.035), y = 2g - 1.02)
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
    const float3 I = eye_to / dist;

    // Geometric specular anti-aliasing (Tokuyoshi & Kaplanyan 2019): widen the lobe where the normal changes quickly across
    // a pixel, so thin edges and normal-map detail don't sparkle as the model moves
    const float3 dndx = ddx(N), dndy = ddy(N);
    const float variance = 0.25 * (dot(dndx, dndx) + dot(dndy, dndy));
    const float a2 = saturate(rough * rough * rough * rough + min(2.0 * variance, 0.18));
    const float rough_aa = sqrt(sqrt(a2));          // perceptual, for the cube's blur

    Surf s;
    s.N = N;
    s.Ng = normalize(i.nrm);
    s.Nb = N;
    s.wrap = 0;
    s.tint = 1;
#if LIGHTING == 4
#ifdef HAS_XfSamplerAdditionalMap
    const float4 sss_map = tex2D(XfSamplerAdditionalMap, i.uv2);
#else
    const float4 sss_map = 1.0;
#endif
#ifdef HAS_gXfLightMapScale
    const float3 scatter = saturate(gXfLightMapScale);
#else
    const float3 scatter = float3(0.5, 0.2, 0.12);
#endif
    const float scatter_max = max(max(scatter.r, scatter.g), max(scatter.b, 1e-3));
    s.wrap = scatter_max * K_SSS_WIDTH * sss_map.g;
    s.tint = lerp(1.0, scatter / scatter_max, K_SSS_SATURATION);
    s.Nb = normalize(lerp(N, s.Ng, 0.5 * sss_map.g));
#endif
    s.V = -I;
    s.nv = abs(dot(N, s.V)) + 1e-4;                 // normal-mapped pixels that face away: not the grazing extreme
    s.a = sqrt(a2);
    s.a2 = a2;
    s.f0 = lerp(knobs.zzz, albedo, metal);

    // ---- direct light ----
    float3 diff = 0, spec = 0, back = 0;
#ifdef HAS_gXfLightParam
    if (gXfLightEnable0) {
        spot(diff, spec, back, s, i.wpos, 0);
        if (gXfLightEnable1) {
            spot(diff, spec, back, s, i.wpos, 1);
            if (gXfLightEnable2) {
                spot(diff, spec, back, s, i.wpos, 2);
                if (gXfLightEnable3) spot(diff, spec, back, s, i.wpos, 3);
            }
        }
    }
#endif
#ifdef HAS_gXfLightSimple
    if (gXfSimpleLightEnable0) {
        simple(diff, spec, back, s, i.wpos, 0);
        if (gXfSimpleLightEnable1) {
            simple(diff, spec, back, s, i.wpos, 1);
            if (gXfSimpleLightEnable2) {
                simple(diff, spec, back, s, i.wpos, 2);
                if (gXfSimpleLightEnable3) simple(diff, spec, back, s, i.wpos, 3);
            }
        }
    }
#endif
    diff *= tw;
    spec *= tw;

    // ---- ambient ----
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
    float3 amb = 0;
    float spec_occ = 1.0;
#ifdef HAS_gXfSHc
    amb += to_lin(sh(N));
#endif
#if LIGHTMAP == 5
    amb += to_lin(i.vcol * lms) * (1.0 + ao_bias * (tw - 1.0));
    spec_occ = i.vcol.r;
#elif LIGHTMAP == 1 || LIGHTMAP == 2
    const float4 lm = tex2D(XfSamplerLightMap, i.uv3);
    amb += to_lin(lm.rgb * lm.a * lms) * (1.0 + ao_bias * (tw - 1.0));
#if LIGHTMAP == 2
    spec_occ = tex2D(XfSamplerShadowMap, i.uv3).r;
#endif
#endif
    spec *= spec_occ;

    // ---- the stage shadow (see the functions above main) ----
    float shadow_dark = 0;      // how far the stage shadow darkens this pixel, before its darkness (0 .. 1)
#if PBR_SHADOW
    if (gPbrShadow[1].x > 0.5 && dist < gPbrShadow[2].z) {
        const float3 Ls = gPbrShadow[5].xyz;
        const float turn = 6.2831853 * frac(52.9829189 * frac(dot(i.vpos, float2(0.06711056, 0.00583715))));
        float min_texels = gPbrShadow[0].z;
#if LIGHTING == 4
        min_texels *= 1.0 + 3.0 * s.wrap;       // skin: the light scattering under it softens the shadow's edge
#endif
        // the cascade by distance from the eye (the stock pass uses view depth: a little coarser at the screen's edges)
        float sh, fade = 1.0;
        if (dist < gPbrShadow[2].x) {
            sh = shd_cascade(0, gPbrShadowNear, i.wpos, Ls, turn, min_texels);
        } else if (dist < gPbrShadow[2].y) {
            sh = shd_cascade(1, gPbrShadowMiddle, i.wpos, Ls, turn, min_texels);
        } else {
            sh = shd_cascade(2, gPbrShadowFar, i.wpos, Ls, turn, min_texels);
            fade = 1.0 - saturate(dist * gPbrShadow[3].x + gPbrShadow[3].y);
        }
        // the stock CASTER receiver: faces turning from the shadow's light darken gradually (none of the light's own)
        const float ndl = dot(s.Ng, -Ls);
        if (gPbrShadow[1].z > 0) sh = max(sh, 1.0 - saturate(ndl * gPbrShadow[1].z));
        else if (ndl < 0) sh = 0;
        shadow_dark = sh * fade;
    }
    spec *= 1.0 - shadow_dark;                  // no highlights where the shadow darkens
#endif

    float3 emit = 0;
#if LIGHTING == 3 || LIGHTING == 4
    const float add_r = tex2D(XfSamplerAdditionalMap, i.uv2).r;
#endif
#if LIGHTING == 3
    emit = to_lin(add_r * lms);                 // EMITSH4SPOT: emission in light units, times the albedo
#elif LIGHTING == 4
#ifdef HAS_gXfSHc
    back += to_lin(sh(-s.Ng)) * 0.25;           // a little of the ambient from behind
#endif
    // THINSH4SPOT: the light coming through, by thinness (1 - AdditionalMap R) and the scatter colour, times the albedo
    emit = back * (1.0 - add_r) * sss_map.g * K_TRANSMISSION * to_lin(lms);
#if PBR_SHADOW
    emit *= 1.0 - shadow_dark;                  // nor light through thin parts
#endif
#endif
)hlsl"
R"hlsl(
    // ---- reflections ----
#ifdef HAS_gXfEnvMapFactor
    const float lod_range = max(gXfEnvMapFactor, 0.0);
#else
    const float lod_range = 0.0;
#endif
    const float3 R = I - 2.0 * dot(N, I) * N;
    float3 refl = 0;
#ifdef HAS_XfSamplerEnvironmentMap
    // A negative reflection strength (gXfFresnelFactor.w, the record's EnvMapPower) asks for the material's own cube as it
    // is, whatever PBR_REFLECT says: the PBR test spheres (PbrSpheres.cpp) put a stage cube there, since roughness only
    // shows in a reflection with detail (the scene's SH, the default, is equally smooth at every roughness).
    if (knobs.w < 0.0) {
        refl = cube_radiance(R, rough_aa, lod_range) * env_brdf(s.f0, rough_aa, s.nv) *
               (-knobs.w * spec_occlusion(s.nv, ao, rough_aa) * spec_occ);
    } else
#endif
    {
    // What reflects. The cube is rarely of the place: every material without one gets DefaultCube_CM, a 64 x 64 daylight
    // sky (decoded mean ~0.6, hot spots up to 8), which made 2B look chrome on any stage.
    //   0: the scene's ambient light (the SH the game gives every SH4SPOT material), read in the reflection direction: the
    //      stage's own colours and level, smooth, no sky. Without SH (4SPOT) the cube relit as in 1.
    //   1: the cube divided by its own mean and scaled to the ambient light at this point: its shape, the scene's level.
    //   2: the cube as it is (a material with a cube made for its place).
#if PBR_REFLECT == 0 && defined(HAS_gXfSHc)
    refl = to_lin(sh(R)) * env_brdf(s.f0, rough_aa, s.nv) * (knobs.w * spec_occlusion(s.nv, ao, rough_aa) * spec_occ);
#elif defined(HAS_XfSamplerEnvironmentMap)
    refl = cube_radiance(R, rough_aa, lod_range) * env_brdf(s.f0, rough_aa, s.nv) *
           (knobs.w * spec_occlusion(s.nv, ao, rough_aa) * spec_occ);
#if PBR_REFLECT != 2
    // Relit: the cube over its own mean (its smallest mip, one lookup per face so no seam) times the ambient light this
    // point gets (SH / vertex colour / lightmap; in the stock shaders' units a white surface under uniform radiance L
    // shows L).
    float3 cube_mean = 0;
    const float3 dirs[6] = { float3(1, 0, 0), float3(-1, 0, 0), float3(0, 1, 0), float3(0, -1, 0), float3(0, 0, 1), float3(0, 0, -1) };
    for (int k = 0; k < 6; ++k) {
        const float4 m = texCUBElod(XfSamplerEnvironmentMap, float4(dirs[k], 16.0));
        cube_mean += to_lin(m.rgb) / max(m.a, 1.0 / 255.0);
    }
    const float3 luma = float3(0.2126, 0.7152, 0.0722);
    refl *= dot(amb, luma) / max(dot(cube_mean / 6.0, luma), 1e-3);
#endif
#endif
    }

    refl *= K_REFLECT_STRENGTH;
    const float3 kd = (1.0 - metal) * albedo;
    float3 col = kd * (diff + amb * ao) + albedo * emit + spec + refl;

#if PBR_DEBUG == 1
    col = albedo;
#elif PBR_DEBUG == 2
    col = to_lin(N * 0.5 + 0.5);
#elif PBR_DEBUG == 3
    col = to_lin(ramp(rough));
#elif PBR_DEBUG == 4
    col = to_lin(ramp(metal));
#elif PBR_DEBUG == 5
    col = to_lin(ramp(ao));
#elif PBR_DEBUG == 6
    col = diff + amb;
#elif PBR_DEBUG == 7
    col = spec + refl;
#endif

    // ---- out: gamma, fog, the "LDR x 2" encoding ----
    col = to_out(col);
#if PBR_SHADOW && PBR_DEBUG == 0
    // the stock receiver's blend, here: toward the shadow colour by shadow x darkness (fogged below with the pixel, as the
    // stock pass fogs its colour by the same factor)
    col = lerp(col, gPbrShadow[4].rgb, shadow_dark * gPbrShadow[1].w);
#endif
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
