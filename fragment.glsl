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

// 2D Simplex Noise for Accretion Plasma Turbulence
vec3 permute(vec3 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

float snoise(vec2 v){
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                        -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy) );
    vec2 x0 = v -   i + dot(i, C.xx);
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod(i, 289.0);
    vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
        + i.x + vec3(0.0, i1.x, 1.0 ));
    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    vec3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

// FBM Noise for Multi-Layer Spiral Plasma Detail
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

// Background Starfield Generator
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

vec3 getStarfield(vec3 rayDir) {
    vec3 dir = normalize(rayDir);
    float n = hash(floor(dir * 500.0));
    if (n > 0.985) {
        float rawVal = (n - 0.985) / 0.015;
        return vec3(pow(rawVal, 2.5) * 1.2);
    }
    return vec3(0.0);
}

void main() {
    vec2 st = (gl_FragCoord.xy - 0.5 * u_resolution) / u_resolution.y;

    // Standard pinhole camera ray direction
    vec3 rayDir = normalize(st.x * u_camRight + st.y * u_camUp + 1.5 * u_camForward);
    vec3 rayPos = u_camPos;

    float Rs = 2.0 * u_mass;
    
    vec3 diskColorAccum = vec3(0.0);
    float alphaAccum = 0.0;
    bool hitEventHorizon = false;

    for (int i = 0; i < 300; i++) {
        float r = length(rayPos);

        // Shadow boundary (Photon sphere / Event horizon interaction)
        if (r < Rs) {
            hitEventHorizon = true;
            break;
        }

        if (r > 100.0) break;

        // Dynamic step size scaling to prevent ray tunneling
        float stepSize = 0.05 + 0.05 * (r / Rs);

        vec3 nextPos = rayPos + rayDir * stepSize;

        // Equatorial Plane Crossing (Accretion Disk)
        if ((rayPos.y * nextPos.y) < 0.0) {
            float t = -rayPos.y / rayDir.y;
            vec3 hitP = rayPos + rayDir * t;
            float hitDist = length(hitP);

            if (hitDist > 1.5 * Rs && hitDist < 10.0 * Rs) {
                float normDist = (hitDist - 1.5 * Rs) / (8.5 * Rs);

                float angle = atan(hitP.z, hitP.x);
                vec2 noiseUV = vec2(hitDist * 0.5 - u_time * 0.1, angle * 2.0);
                
                float plasmaNoise = fbm(noiseUV * 2.0) * 0.5 + 0.5;
                float intensity = exp(-normDist * 2.5) * (0.2 + 0.8 * plasmaNoise);

                // Red and Orange Gradient Definition
                vec3 brightOrange = vec3(3.5, 1.2, 0.15); // Hot inner rim
                vec3 fieryRed     = vec3(2.0, 0.2, 0.02); // Mid disk
                vec3 deepCrimson  = vec3(0.6, 0.02, 0.005); // Outer rim

                vec3 colorGradient;
                if (normDist < 0.3) {
                    colorGradient = mix(brightOrange, fieryRed, normDist / 0.3);
                } else {
                    colorGradient = mix(fieryRed, deepCrimson, (normDist - 0.3) / 0.7);
                }

                float alphaVal = clamp(intensity * 0.4, 0.0, 1.0);

                // Removed Doppler factor for uniform radial brightness
                diskColorAccum += (1.0 - alphaAccum) * colorGradient * intensity * alphaVal;
                alphaAccum += alphaVal;

                if (alphaAccum >= 0.98) break;
            }
        }

        // Relativistic geodesic ray bending
        vec3 L = cross(rayPos, rayDir);
        float h2 = dot(L, L);
        vec3 accel = -1.5 * Rs * h2 * rayPos / pow(r, 5.0);
        
        rayDir = normalize(rayDir + accel * stepSize);
        rayPos = nextPos;
    }

    vec3 outColor = diskColorAccum;

    if (!hitEventHorizon) {
        outColor += (1.0 - alphaAccum) * getStarfield(rayDir);
    }

    // Filmic Tonemapping
    outColor = vec3(1.0) - exp(-outColor * 1.2);

    FragColor = vec4(outColor, 1.0);
}
