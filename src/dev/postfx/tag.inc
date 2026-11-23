// The depth pass with object IDs (Tags.cpp includes it as one string). In the game, Write file puts a copy in
// nativePC\shader\postfx\tag.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// tag.hlsl: dmc4_hook PostFX object IDs, in place of tXfMaterialZPass's two pixel shaders (XfMaterialBase.bfx;
// Vibed\RE\depth_texture.md). They write the frame's depth texture exactly as the stock ones do (clip z / w packed into
// A / R / G) and the drawing model's ID into blue, which the stock shaders leave 0: PostFX sets c220.x = ID / 255 before
// each depth-pass draw (Tags.cpp finds the model by its command records), 0 for geometry no model drew.

float4 gTagId : register(c220);

// the stock packing: a = floor(z x 256) / 255, r = floor(frac(z x 256) x 256) / 255, g = the remainder
float4 pack(float z) {
    const float x = z * 256.0;
    const float hi = x - frac(x);
    const float y = frac(x) * 256.0;
    const float mid = y - frac(y);
    return float4(mid * 0.00392156886, frac(y), gTagId.x, hi * 0.00392156886);
}

// opaque materials (stock record 0): the depth from TEXCOORD0.zw
float4 ps_opaque(float4 t0 : TEXCOORD0) : COLOR0 {
    return pack(t0.z / t0.w);
}

// alpha-tested materials (stock record 1): cut where the albedo's alpha is under the material's cutoff, depth from
// TEXCOORD1.zw
sampler2D XfSamplerAlbedoMap : register(s0);
float gXfBlendFactor : register(c0);
float4 ps_alpha(float4 t0 : TEXCOORD0, float4 t1 : TEXCOORD1) : COLOR0 {
    clip(tex2D(XfSamplerAlbedoMap, t0.xy).a - gXfBlendFactor);
    return pack(t1.z / t1.w);
}
)hlsl"
