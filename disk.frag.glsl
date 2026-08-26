#version 330 core

in vec3 particleColor;
out vec4 FragColor;

void main()
{
    // Convert point coordinates into a circular sprite
    vec2 circCoord = gl_PointCoord - vec2(0.5);
    float distSq = dot(circCoord, circCoord);

    // Discard pixels outside the radius to make point circular
    if (distSq > 0.25) {
        discard;
    }

    // Radial alpha falloff for a soft glowing edge
    float alpha = 1.0 - smoothstep(0.15, 0.25, distSq);

    FragColor = vec4(particleColor, alpha);
}
