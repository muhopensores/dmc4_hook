// The ambient occlusion pixel shaders' source (PostFX.cpp includes it as one string). In the game, Write file puts a copy in
// nativePC\shader\postfx\ao.hlsl, which is then used instead (Reload recompiles it).
R"hlsl(// ao.hlsl: dmc4_hook PostFX ambient occlusion (Vibed\PostFX\README.md). Two pixel shaders over a full-screen triangle,
// drawn after the opaque scene and before anything transparent:
//   ps_ao    occlusion from the frame's depth texture into an 8-bit target of its own;
//   ps_blur  a depth-aware 4 x 4 blur of it (the sample pattern repeats every 4 x 4 pixels), multiplied onto the scene.
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

float4 ps_ao(float2 uv : TEXCOORD0, float2 vpos : VPOS) : COLOR0 {
    float z = raw_depth(uv);
    if (z >= 0.99999) return 1;                              // nothing drawn here (sky)
    float3 p = position(uv, z);
    float fade = saturate((gAoParam2.x - p.z) / max(gAoParam2.x - gAoParam.w, 1.0));
    if (fade <= 0) return 1;

    // normal from the nearer neighbour on each axis, so it doesn't smear across silhouettes
    float3 px1 = position_at(uv + float2(gScreen.x, 0)), px0 = position_at(uv - float2(gScreen.x, 0));
    float3 py1 = position_at(uv - float2(0, gScreen.y)), py0 = position_at(uv + float2(0, gScreen.y));
    float3 dx = abs(px1.z - p.z) < abs(p.z - px0.z) ? px1 - p : p - px0;
    float3 dy = abs(py1.z - p.z) < abs(p.z - py0.z) ? py1 - p : p - py0;
    float3 n = normalize(cross(dx, dy));
    if (dot(n, p) > 0) n = -n;                               // facing the camera

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
)hlsl"
