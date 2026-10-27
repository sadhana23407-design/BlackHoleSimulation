#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_camPos;
uniform vec3 u_camForward;
uniform vec3 u_camUp;
uniform vec3 u_camRight;
uniform float u_mass;

// ============================================================
//  TUNABLES  (radii are in units of the Schwarzschild radius Rs)
// ============================================================
const float PI              = 3.14159265359;
const float BOUND           = 30.0;     // ray-tracing sphere radius
const int   MAX_STEPS       = 350;

const float DISK_INNER      = 3.0;      // ISCO of a Schwarzschild BH = 3 Rs (6M)
const float DISK_OUTER      = 12.0;
const float DISK_TEMP_INNER = 7000.0;   // Kelvin at the inner edge
const float DISK_TEMP_REF   = 6000.0;   // brightness reference temperature
const float DISK_GAIN       = 2.0;      // overall disk brightness
const float DISK_SPIN       = 0.6;      // rad/s at the inner edge (Keplerian falloff outward)

const float GLOW_GAIN       = 0.3;      // photon-ring / lensed-light haze
const float EXPOSURE        = 1.1;

float gRs;  // Schwarzschild radius, set in main()


vec3 permute(vec3 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }

float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                        -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);
    vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod(i, 289.0);
    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0))
                     + i.x + vec3(0.0, i1.x, 1.0));
    vec3 m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0);
    m = m * m;
    m = m * m;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);
    vec3 g;
    g.x  = a0.x * x0.x + h.x * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * snoise(st);
        st *= 2.1;
        amplitude *= 0.5;
    }
    return value;
}

float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

// ============================================================
//  BACKGROUND: round, coloured, jittered stars + faint galactic band
// ============================================================
vec3 starLayer(vec3 dir, float scale, float density, float size) {
    vec3 p  = dir * scale;
    vec3 id = floor(p);
    vec3 f  = fract(p);

    float h = hash(id);
    float thr = 1.0 - density;
    if (h < thr) return vec3(0.0);

    // jitter kept within [0.2, 0.8] so stars never clip at cell borders
    vec3 center = 0.5 + 0.6 * (vec3(hash(id + 13.1), hash(id + 27.7), hash(id + 41.3)) - 0.5);
    float d = length(f - center);
    float core = smoothstep(size, 0.0, d);

    float mag = (h - thr) / density;                         // 0..1
    vec3 tint = mix(vec3(1.0, 0.70, 0.50), vec3(0.60, 0.75, 1.0), hash(id + 5.5));
    return tint * core * (0.25 + 3.0 * mag * mag * mag);
}

vec3 getStarfield(vec3 rayDir) {
    vec3 dir = normalize(rayDir);

    vec3 stars = starLayer(dir, 250.0, 0.06, 0.22)
               + starLayer(dir, 110.0, 0.05, 0.28) * 1.5;

    // keep the band only as a subtle brightness boost for stars along it
    float band = exp(-pow(dot(dir, normalize(vec3(0.25, 1.0, 0.35))) * 3.0, 2.0));

    return stars * (1.0 + band);
}

// ============================================================
//  COLOUR: blackbody radiation (Kelvin -> linear RGB)
// ============================================================
vec3 blackbody(float T) {
    float t = clamp(T, 1000.0, 40000.0) / 100.0;
    vec3 c;
    c.r = (t <= 66.0) ? 1.0 : 1.292936186 * pow(t - 60.0, -0.1332047592);
    c.g = (t <= 66.0) ? 0.3900815788 * log(t) - 0.6318414438
                      : 1.129890861 * pow(t - 60.0, -0.0755148492);
    c.b = (t >= 66.0) ? 1.0 : ((t <= 19.0) ? 0.0 : 0.5432067891 * log(t - 10.0) - 1.196254089);
    return pow(clamp(c, 0.0, 1.0), vec3(2.2));   // sRGB -> linear
}

// Orange-red disk palette driven by the observed (Doppler/redshift-shifted)
// temperature: cool/receding gas -> deep crimson, hot/approaching gas -> bright orange.
// x = 0 at the inner edge, 1 at the outer edge. Same colour all the way round.
vec3 diskPalette(float x) {
    const vec3 orange = vec3(1.00, 0.45, 0.06);
    const vec3 red    = vec3(0.90, 0.07, 0.01);
    return mix(orange, red, smoothstep(0.0, 1.0, x));
}

// ============================================================
//  GEODESIC INTEGRATION
//  Schwarzschild null geodesic in "Binet form": a = -1.5 Rs h^2 r / r^5
//  h^2 = |r x v|^2 is conserved, speed is NOT 1 along the path, so we
//  do not renormalise v (the old code did, which slightly distorts orbits).
// ============================================================
vec3 accel(vec3 p, float h2) {
    float r2 = dot(p, p);
    float r  = sqrt(r2);
    return -1.5 * gRs * h2 * p / (r2 * r2 * r);
}

void rk4(inout vec3 p, inout vec3 v, float dt, float h2) {
    vec3 k1x = v;
    vec3 k1v = accel(p, h2);
    vec3 k2x = v + 0.5 * dt * k1v;
    vec3 k2v = accel(p + 0.5 * dt * k1x, h2);
    vec3 k3x = v + 0.5 * dt * k2v;
    vec3 k3v = accel(p + 0.5 * dt * k2x, h2);
    vec3 k4x = v + dt * k3v;
    vec3 k4v = accel(p + dt * k3x, h2);
    p += dt / 6.0 * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
    v += dt / 6.0 * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}

// ============================================================
//  ACCRETION DISK
//  - thin disk starting at the ISCO
//  - Keplerian differential rotation (inner parts spin faster -> shear/spirals)
//  - temperature profile T ~ r^-3/4
//  - relativistic Doppler beaming + gravitational redshift -> the bright
//    approaching side and the dim, redder receding side
// ============================================================
vec3 shadeDisk(vec3 hit, vec3 dirTraced, float rr, out float alpha) {
    float x = (rr - DISK_INNER) / (DISK_OUTER - DISK_INNER);

    // ---- turbulence, periodic in angle (no atan seam) ----
    float phi = atan(hit.z, hit.x);
    float omega = DISK_SPIN * pow(DISK_INNER / rr, 1.5);
    phi = phi - omega * u_time + 2.0 * log(rr / DISK_INNER);   // shear + static spiral twist
    float a = fract(phi / (2.0 * PI));
    const float N = 20.0;                                        // integer => seamless
    float rad = rr * 2.5;
    float n = mix(fbm(vec2(rad, a * N)), fbm(vec2(rad, (a - 1.0) * N)), a) * 0.5 + 0.5;

    // ---- even lighting: no Doppler, no redshift ----
    // colour depends only on radius; brightness only gets gentle turbulence
    vec3 col = diskPalette(clamp(x, 0.0, 1.0));
    float bright = (0.8 + 0.4 * n) * mix(1.0, 0.6, x);

    // soft inner/outer edges, patchy opacity
    float edge = smoothstep(0.0, 0.04, x) * (1.0 - smoothstep(0.80, 1.0, x));
    alpha = clamp(edge * mix(0.25, 0.9, smoothstep(0.2, 0.8, n)), 0.0, 1.0);

    return col * bright * DISK_GAIN;
}

// ============================================================
//  OUTPUT: tone map + gamma + dither
// ============================================================
vec3 finish(vec3 c) {
    c = vec3(1.0) - exp(-c * EXPOSURE);
    c = pow(c, vec3(1.0 / 2.2));
    c += (hash(vec3(gl_FragCoord.xy, fract(u_time))) - 0.5) / 255.0;
    return c;
}

void main() {
    vec2 st = (gl_FragCoord.xy - 0.5 * u_resolution) / u_resolution.y;

    vec3 rd = normalize(st.x * u_camRight + st.y * u_camUp + 1.5 * u_camForward);
    vec3 p  = u_camPos;

    gRs = 2.0 * u_mass;
    float Rs = gRs;
    float boundR = BOUND * Rs;

    // Jump to the bounding sphere if the camera is outside it
    if (length(p) > boundR) {
        float b = dot(p, rd);
        float c = dot(p, p) - boundR * boundR;
        float disc = b * b - c;
        if (disc < 0.0 || b > 0.0) {
            FragColor = vec4(finish(getStarfield(rd)), 1.0);
            return;
        }
        p += rd * (-b - sqrt(disc) + 0.001);
    }

    vec3 v = rd;
    vec3 L = cross(p, v);
    float h2 = dot(L, L);                    // conserved along the ray

    vec3 diskAcc = vec3(0.0);
    vec3 glowAcc = vec3(0.0);
    float trans = 1.0;                       // transmittance through the disk
    bool hitHorizon = false;

    const vec3 glowColor = vec3(1.0, 0.35, 0.06);

    for (int i = 0; i < MAX_STEPS; i++) {
        float r = length(p);

        if (r < Rs) { hitHorizon = true; break; }
        if (r > boundR + 1.0) break;

        // adaptive step: small near the hole, large far away
        float stepLen = clamp(0.12 * (r - 0.9 * Rs), 0.02 * Rs, 1.5 * Rs);
        float dt = stepLen / max(length(v), 0.2);

        // volumetric haze strongest near the photon sphere (~1.5 Rs)
        float rr = r / Rs;
        float gl = GLOW_GAIN * (stepLen / Rs) / (0.3 + rr * rr * rr);
        glowAcc += trans * glowColor * gl;

        vec3 pn = p, vn = v;
        rk4(pn, vn, dt, h2);

        // disk plane crossing (y = 0)
        if (p.y * pn.y < 0.0) {
            float t = p.y / (p.y - pn.y);
            vec3 hit = mix(p, pn, t);
            float hr = length(hit) / Rs;
            if (hr > DISK_INNER && hr < DISK_OUTER) {
                vec3 vh = normalize(mix(v, vn, t));
                float alpha;
                vec3 em = shadeDisk(hit, vh, hr, alpha);
                diskAcc += trans * alpha * em;
                trans *= (1.0 - alpha);
                if (trans < 0.02) break;
            }
        }

        p = pn;
        v = vn;
    }

    vec3 outColor = diskAcc;

    if (!hitHorizon) {
        // glow + stars only from rays that escape, so the shadow stays pure black
        outColor += glowAcc;
        outColor += trans * getStarfield(v);
    }

    FragColor = vec4(finish(outColor), 1.0);
}
