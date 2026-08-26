#version 330 core

in float Brightness;

out vec4 FragColor;

void main()
{
    FragColor = vec4(
        Brightness,
        Brightness,
        Brightness,
        1.0
    );
}