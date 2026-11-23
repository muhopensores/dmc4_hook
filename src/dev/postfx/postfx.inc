// The post effects' pixel shaders (PostFX.cpp includes them as one string). In the game, Write file puts a copy in
// nativePC\shader\postfx\postfx.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// postfx.hlsl: dmc4_hook PostFX (Vibed\PostFX\README.md). Pixel shaders over a full-screen triangle, drawn after the
// opaque scene and before anything transparent:
//   ps_ao       ambient occlusion from the frame's depth texture into an 8-bit target of its own;
//   ps_blur     a depth-aware 4 x 4 blur of it (the sample pattern repeats every 4 x 4 pixels), multiplied onto the scene;
//   ps_outline  ink lines from the depth (silhouettes and creases), multiplied onto the scene;
//   ps_contact  contact shadows: a short march toward the sun through the depth, multiplied onto the scene;
//   ps_fxaa     anti-aliasing (FXAA 3.11's quality algorithm, Lottes 2011) from a copy of the scene, written over it.
// The depth texture (sRender +0x1700C, Vibed\RE\depth_texture.md) holds clip z / w packed into A (top byte), R and G.
// Positions are rebuilt in a camera space of the screen's own axes (x right, y up, z = distance ahead), so the
// projection's sign conventions don't matter.

sampler2D gDepth : register(s0);    // point, clamp
sampler2D gAo    : register(s1);    // point, clamp (ps_blur)

float4 gProj   : register(c0);      // P00, P11, P22, P32 of the camera's projection (row vectors: clip = view x P)
float4 gProj2  : register(c1);      // P23, -, -, -
float4 gScreen : register(c2);      // 1 / width, 1 / height, width, height
float4 gAoParam: register(c3);      // radius (world units, cm), strength, bias, fade start (distance)
float4 gAoParam2: register(c4);     // fade end (distance), max screen radius (uv), view (0 picture, 1 AO, 2 depth), -
float4 gLine    : register(c5);     // offset (px), silhouette threshold, crease threshold, opacity
float4 gLineColor: register(c6);    // rgb, the distance (cm) where lines are gone
float4 gContact : register(c7);     // toward the sun in this camera space (x right, y up, z ahead), ray length (cm)
float4 gContact2: register(c8);     // thickness (cm), darkness, the distance (cm) where they're gone, -
float4 gAA      : register(c9);     // subpixel softening (0 .. 1), edge threshold, darkest edge threshold, -
sampler2D gScene : register(s2);    // the scene's copy (linear, clamp; ps_fxaa)

static const int kSamples = 16;

float raw_depth(float2 uv) {
    float4 t = tex2Dlod(gDepth, float4(uv, 0, 0));
    return dot(t.rga, float3(0.00389099144, 1.51991853e-005, 0.99609381));
}

// distance ahead of the camera for a stored depth
float distance_of(float z) { return abs(gProj.w / (z * gProj2.x - gProj.z)); }

float3 position(float2 uv, float z) {
    float d = distance_of(z);
    float2 ndc = uv * float2(2, -2) + float2(-1, 1);
    return float3(ndc.x * d / abs(gProj.x), ndc.y * d / abs(gProj.y), d);
}

float3 position_at(float2 uv) { return position(uv, raw_depth(uv)); }

// the surface normal from the nearer neighbour on each axis, so it doesn't smear across silhouettes; facing the camera
float3 normal_at(float2 uv, float3 p) {
    float3 px1 = position_at(uv + float2(gScreen.x, 0)), px0 = position_at(uv - float2(gScreen.x, 0));
    float3 py1 = position_at(uv - float2(0, gScreen.y)), py0 = position_at(uv + float2(0, gScreen.y));
    float3 dx = abs(px1.z - p.z) < abs(p.z - px0.z) ? px1 - p : p - px0;
    float3 dy = abs(py1.z - p.z) < abs(p.z - py0.z) ? py1 - p : p - py0;
    float3 n = normalize(cross(dx, dy));
    return dot(n, p) > 0 ? -n : n;
}

// a camera-space point to its screen uv
float2 uv_of(float3 q) {
    float2 ndc = float2(q.x * abs(gProj.x), q.y * abs(gProj.y)) / q.z;
    return float2(0.5 + 0.5 * ndc.x, 0.5 - 0.5 * ndc.y);
}

float4 ps_ao(float2 uv : TEXCOORD0, float2 vpos : VPOS) : COLOR0 {
    float z = raw_depth(uv);
    if (z >= 0.99999) return 1;                              // nothing drawn here (sky)
    float3 p = position(uv, z);
    float fade = saturate((gAoParam2.x - p.z) / max(gAoParam2.x - gAoParam.w, 1.0));
    if (fade <= 0) return 1;

    float3 n = normal_at(uv, p);

    float radius = gAoParam.x;
    float2 r_uv = radius * 0.5 * abs(gProj.xy) / p.z;        // the radius on screen at this distance
    float k = min(1.0, gAoParam2.y / max(r_uv.x, r_uv.y));   // near the camera the disk would cover the screen
    r_uv *= k;
    radius *= k;

    // a 16-point spiral, turned per pixel by a 4 x 4 pattern (the blur averages it out)
    float2 cell = fmod(vpos, 4.0);
    float turn = (cell.x * 4.0 + cell.y) * (6.2831853 / 16.0) + cell.x * 0.7;
    float sum = 0, count = 0;
    for (int i = 0; i < kSamples; ++i) {
        float a = i * 2.3999632 + turn;
        float s = sqrt((i + 0.5) / kSamples);
        float2 o = float2(cos(a), sin(a)) * s * r_uv * float2(1, -1);
        float2 t = uv + o;
        if (any(t != saturate(t))) continue;                 // off screen: the clamped edge would occlude
        count += 1;
        float3 q = position_at(t);
        float3 v = q - p;
        float dd = dot(v, v);
        float occ = max(0, dot(n, v) * rsqrt(dd + 1e-4) - gAoParam.z);
        sum += occ * saturate(1 - dd / (radius * radius));
    }
    float ao = saturate(1 - gAoParam.y * sum / max(count, 1));
    ao = lerp(1, ao, fade);
    return float4(ao, ao, ao, 1);
}

float4 ps_blur(float2 uv : TEXCOORD0) : COLOR0 {
    float dc = distance_of(raw_depth(uv));
    if (gAoParam2.z > 1.5) { float g = frac(dc / 1000.0); return float4(g, g, g, 1); }   // depth: bands every 10 m
    float sum = 0, wsum = 0;
    for (int y = -2; y < 2; ++y) {
        for (int x = -2; x < 2; ++x) {
            float2 t = uv + float2(x, y) * gScreen.xy;          // 4 x 4 texels: the pattern's whole cell
            float ds = distance_of(raw_depth(t));
            float w = 1.0 / (0.001 + abs(ds - dc) / dc * 50.0);   // samples on another surface count little
            sum += tex2Dlod(gAo, float4(t, 0, 0)).r * w;
            wsum += w;
        }
    }
    float ao = sum / wsum;
    return float4(ao, ao, ao, 1);
}
// Ink lines. w = 1 / distance is linear across the screen on any plane, so its second difference is 0 on flat surfaces and
// jumps at creases; a neighbour much farther away is a silhouette, and the line goes on the nearer (front) side.
float4 ps_outline(float2 uv : TEXCOORD0) : COLOR0 {
    float z = raw_depth(uv);
    if (z >= 0.99999) return 1;
    float d = distance_of(z);
    float fade = saturate((gLineColor.w - d) / (0.4 * gLineColor.w));
    if (fade <= 0) return 1;
    float2 o = gLine.x * gScreen.xy;
    float dl = distance_of(raw_depth(uv - float2(o.x, 0))), dr = distance_of(raw_depth(uv + float2(o.x, 0)));
    float du = distance_of(raw_depth(uv - float2(0, o.y))), dd = distance_of(raw_depth(uv + float2(0, o.y)));
    float far = max(max(dl, dr), max(du, dd));
    float sil = saturate(((far - d) / d - gLine.y) / gLine.y);
    float w = 1 / d;
    float cx = abs(1 / dl + 1 / dr - 2 * w) / w, cy = abs(1 / du + 1 / dd - 2 * w) / w;
    float crease = sil > 0 ? 0 : saturate((max(cx, cy) - gLine.z) / gLine.z);
    float ink = max(sil, crease) * fade * gLine.w;
    return float4(lerp(float3(1, 1, 1), gLineColor.rgb, ink), 1);
}
// Contact shadows: from the pixel (lifted off its surface) toward the sun in 16 steps; the first step that lands behind
// the depth by less than the thickness is a nearby occluder. Nearer hits shade more. Surfaces facing away from the sun
// are left alone (the lighting already shades them).
float4 ps_contact(float2 uv : TEXCOORD0) : COLOR0 {
    float z = raw_depth(uv);
    if (z >= 0.99999) return 1;
    float3 p = position(uv, z);
    float fade = saturate((gContact2.z - p.z) / (0.4 * gContact2.z));
    if (fade <= 0) return 1;
    float3 L = gContact.xyz;
    float3 n = normal_at(uv, p);
    if (dot(n, L) <= 0.05) return 1;
    float3 start = p + n * (0.5 + p.z * 0.002);             // off the surface: depth precision and the normal's error
    float occ = 0;
    [loop] for (int i = 1; i <= 16; ++i) {
        float t = gContact.w * i / 16.0;
        float3 q = start + L * t;
        if (q.z < 15.0) break;                                // behind the near plane
        float2 s = uv_of(q);
        if (any(s != saturate(s))) break;
        float behind = q.z - distance_of(raw_depth(s));
        if (behind > 0.3 + q.z * 0.001 && behind < gContact2.x) {
            occ = 1 - (i - 1) / 16.0;
            break;
        }
    }
    float shade = 1 - gContact2.y * occ * fade;
    return float4(shade, shade, shade, 1);
}
// ---- FXAA ----
// The scene is the game's "LDR x 2" encoding (gamma, up to 2): luma of half of it, clamped. Edges are found by luma
// contrast, followed along their length up to 12 steps each way to find where they end, and the pixel is re-sampled
// across the edge by how far along it sits (plus a little subpixel softening).
float aa_luma(float3 c) { return dot(saturate(c * 0.5), float3(0.299, 0.587, 0.114)); }
float3 aa_rgb(float2 uv) { return tex2Dlod(gScene, float4(uv, 0, 0)).rgb; }
float aa_lum(float2 uv) { return aa_luma(aa_rgb(uv)); }

float4 ps_fxaa(float2 uv : TEXCOORD0) : COLOR0 {
    const float2 px = gScreen.xy;
    const float3 rgbM = aa_rgb(uv);
    const float lM = aa_luma(rgbM);
    const float lN = aa_lum(uv + float2(0, -px.y)), lS = aa_lum(uv + float2(0, px.y));
    const float lW = aa_lum(uv + float2(-px.x, 0)), lE = aa_lum(uv + float2(px.x, 0));
    const float hi = max(max(lN, lS), max(max(lW, lE), lM));
    const float lo = min(min(lN, lS), min(min(lW, lE), lM));
    const float range = hi - lo;
    if (range < max(gAA.z, hi * gAA.y)) return float4(rgbM, 1);

    const float lNW = aa_lum(uv - px), lSE = aa_lum(uv + px);
    const float lNE = aa_lum(uv + float2(px.x, -px.y)), lSW = aa_lum(uv + float2(-px.x, px.y));

    // subpixel: how far the pixel stands out from its neighbourhood
    const float avg = (2.0 * (lN + lS + lW + lE) + (lNW + lNE + lSW + lSE)) / 12.0;
    float sub = saturate(abs(avg - lM) / range);
    sub = smoothstep(0.0, 1.0, sub);
    sub = sub * sub * gAA.x;

    // the edge's orientation
    const float horz = abs(lNW + lSW - 2.0 * lW) + 2.0 * abs(lN + lS - 2.0 * lM) + abs(lNE + lSE - 2.0 * lE);
    const float vert = abs(lNW + lNE - 2.0 * lN) + 2.0 * abs(lW + lE - 2.0 * lM) + abs(lSW + lSE - 2.0 * lS);
    const bool horzSpan = horz >= vert;
    const float l1 = horzSpan ? lN : lW, l2 = horzSpan ? lS : lE;
    const float g1 = l1 - lM, g2 = l2 - lM;
    const bool side1 = abs(g1) >= abs(g2);
    const float gradient = max(abs(g1), abs(g2));
    float stepLen = horzSpan ? px.y : px.x;
    if (side1) stepLen = -stepLen;
    const float localAvg = 0.5 * ((side1 ? l1 : l2) + lM);

    // walk along the edge both ways until the luma leaves it
    float2 posB = uv;
    if (horzSpan) posB.y += stepLen * 0.5; else posB.x += stepLen * 0.5;
    const float2 off = horzSpan ? float2(px.x, 0) : float2(0, px.y);
    float2 posN = posB - off, posP = posB + off;
    const float gScaled = gradient * 0.25;
    float endN = aa_lum(posN) - localAvg, endP = aa_lum(posP) - localAvg;
    bool doneN = abs(endN) >= gScaled, doneP = abs(endP) >= gScaled;
    [unroll] for (int i = 1; i < 12; ++i) {
        const float q = i < 5 ? 1.0 : i == 5 ? 1.5 : i < 10 ? 2.0 : i == 10 ? 4.0 : 8.0;
        if (!doneN) { posN -= off * q; endN = aa_lum(posN) - localAvg; doneN = abs(endN) >= gScaled; }
        if (!doneP) { posP += off * q; endP = aa_lum(posP) - localAvg; doneP = abs(endP) >= gScaled; }
    }
    const float dN = horzSpan ? uv.x - posN.x : uv.y - posN.y;
    const float dP = horzSpan ? posP.x - uv.x : posP.y - uv.y;
    const bool nearN = dN < dP;
    const float endL = nearN ? endN : endP;
    const bool mBelow = lM - localAvg < 0.0;
    const float edgeOfs = ((endL < 0.0) == mBelow) ? 0.0 : 0.5 - min(dN, dP) / (dN + dP);
    const float ofs = max(edgeOfs, sub);
    float2 uvF = uv;
    if (horzSpan) uvF.y += ofs * stepLen; else uvF.x += ofs * stepLen;
    return float4(aa_rgb(uvF), 1);
}
)hlsl"
