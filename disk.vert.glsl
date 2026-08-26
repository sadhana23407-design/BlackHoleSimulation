#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 particleColor;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Transform object 3D position to screen space
    gl_Position = u_projection * u_view * vec4(aPos, 1.0);

    // Scale point size dynamically based on distance to camera
    float dist = gl_Position.w;
    gl_PointSize = clamp(100.0 / dist, 3.0, 20.0);

    particleColor = aColor;
}
