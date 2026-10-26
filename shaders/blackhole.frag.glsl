#version 330 core

out vec4 FragColor;

uniform vec3 u_objectColor;
uniform float u_brightness;

void main()
{
    // Output color modulated by brightness intensity
    FragColor = vec4(u_objectColor * u_brightness, 1.0);
}