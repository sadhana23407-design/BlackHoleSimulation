#version 330 core

out vec4 FragColor;

uniform vec2 u_resolution;
uniform float u_time;
uniform float u_mass;

uniform vec3 u_camPos;
uniform vec3 u_camForward;
uniform vec3 u_camUp;
uniform vec3 u_camRight;

// ------------------------------------------------------------------
// SEAMLESS 3D-BASED NOISE & PLASMA UTILITIES
// ------------------------------------------------------------------

float hash31(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

// 3D Smooth Perlin-like Noise (Eliminates 2D polar wrapping seams)
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i);
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    return mix(
        mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
        mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y),
        f.z
    );
}

// Seamless Fractal Brownian Motion using trigonometric cylinder projection
float seamlessFbm(float r, float angle, float time, float blurFactor) {
    float rotSpeed = 2.5 / sqrt(r);
    float currentAngle = angle + time * rotSpeed;

    // Radius of noise cylinder in 3D noise space
    float cylR = 2.0;
    
    // Project polar coordinates to continuous 3D cylinder space
    vec3 p = vec3(
        cos(currentAngle) * cylR,
        sin(currentAngle) * cylR,
        r * mix(1.5, 0.4, blurFactor)
    );

    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        v += a * noise3D(p);
        p *= 2.02;
        a *= 0.5;
    }

    return mix(v, 0.65, blurFactor * 0.85);
}

vec3 getSkyColor(vec3 rayDir) {
    float starDensity = hash31(floor(rayDir * 180.0));
    float star = pow(starDensity, 32.0) * 2.5;
    vec3 sky = vec3(0.003, 0.003, 0.01) + rayDir * 0.01;
    return sky + vec3(star);
}

// ------------------------------------------------------------------
// 3D VOLUMETRIC ACCRETION DISK
// ------------------------------------------------------------------

float getDiskDensity(vec3 pos, float u_time, float innerR, float outerR, float blurFactor, out float doppler) {
    float r = length(pos.xz);
    if (r < innerR || r > outerR) return 0.0;

    // 3D flare height
    float height = 0.4 * (r / outerR) + 0.1; 
    float verticalFade = exp(-pow(pos.y / height, 2.0));

    if (verticalFade < 0.01) return 0.0;

    float angle = atan(pos.z, pos.x);

    // Compute seamless plasma density
    float n = seamlessFbm(r, angle, u_time, blurFactor);
    float radialFade = smoothstep(innerR, innerR + 0.8, r) * smoothstep(outerR, outerR - 2.5, r);

    vec3 orbitDir = normalize(vec3(-pos.z, 0.0, pos.x));
    doppler = dot(orbitDir, vec3(1.0, 0.0, 0.0));

    return n * radialFade * verticalFade;
}

// ------------------------------------------------------------------
// MAIN RAYMARCHER
// ------------------------------------------------------------------

void main() {
    vec2 st = (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;

    vec3 rayDir = normalize(st.x * u_camRight + st.y * u_camUp + u_camForward);
    vec3 rayPos = u_camPos;

    float Rs = 2.0 * u_mass;
    float eventHorizon = Rs;
    float innerDisk = 2.4 * u_mass;
    float outerDisk = 12.0 * u_mass;

    float camDist = length(u_camPos);
    float distanceBlur = smoothstep(25.0, 150.0, camDist);
    float maxEscapeDist = max(200.0, camDist * 2.0);

    vec3 finalColor = vec3(0.0);
    vec3 accumulatedLight = vec3(0.0);
    float transmittance = 1.0;
    vec3 haloGlow = vec3(0.0);

    int maxSteps = 450;

    for (int i = 0; i < maxSteps; i++) {
        float r = length(rayPos);

        if (r <= eventHorizon) {
            transmittance = 0.0;
            break;
        }

        if (r > maxEscapeDist) {
            finalColor = getSkyColor(rayDir);
            break;
        }

        float stepSize = max(0.015, (r - eventHorizon) * mix(0.035, 0.08, distanceBlur));

        // Atmospheric/Photon Halo (Glows smoothly at long distances)
        if (r < outerDisk * 1.5 && r > eventHorizon) {
            float haloDensity = exp(-(r - eventHorizon) * 0.35);
            vec3 glowColor = mix(vec3(1.0, 0.3, 0.02), vec3(1.0, 0.75, 0.4), haloDensity);
            haloGlow += glowColor * haloDensity * stepSize * mix(0.3, 1.2, distanceBlur) * transmittance;
        }

        // 3D Volumetric Density Sampling
        float dopplerShift = 0.0;
        float density = getDiskDensity(rayPos, u_time, innerDisk, outerDisk, distanceBlur, dopplerShift);

        if (density > 0.001) {
            vec3 coldPlasma = vec3(1.0, 0.25, 0.02);
            vec3 hotPlasma  = vec3(1.0, 0.88, 0.55);
            vec3 blueShift  = vec3(0.6, 0.8, 1.0);

            vec3 baseColor = mix(coldPlasma, hotPlasma, density);
            baseColor = mix(baseColor, blueShift, clamp(dopplerShift * 0.4, 0.0, 0.5));

            float absorption = density * stepSize * 1.8;
            accumulatedLight += baseColor * density * stepSize * 2.2 * transmittance;
            transmittance *= exp(-absorption);

            if (transmittance < 0.01) break;
        }

        // Relativistic Geodesic Curvature
        vec3 L = cross(rayPos, rayDir);
        float h2 = dot(L, L);
        vec3 curvatureAccel = -1.5 * Rs * h2 * normalize(rayPos) / pow(r, 5.0);

        rayDir = normalize(rayDir + curvatureAccel * stepSize);
        rayPos += rayDir * stepSize;
    }

    finalColor = finalColor * transmittance + accumulatedLight + haloGlow;
    FragColor = vec4(finalColor, 1.0);
}