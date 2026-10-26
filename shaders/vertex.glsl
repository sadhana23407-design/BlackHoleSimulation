#version 330 core

layout (location = 0) in vec2 aPos;

out vec2 TexCoords;

void main()
{
    // Map clip space [-1, 1] to texture space [0, 1]
    TexCoords = (aPos + vec2(1.0)) * 0.5;
    
    // Pass quad vertex directly to screen-space clip coordinates
    gl_Position = vec4(aPos, 0.0, 1.0);
}