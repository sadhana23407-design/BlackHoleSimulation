#version 330 core

layout (location = 0) in vec2 aPos;

out vec2 TexCoords;

void main()
{
    // Map screen-space quad [-1.0, 1.0] to texture coordinates [0.0, 1.0]
    TexCoords = (aPos + vec2(1.0)) * 0.5;
    
    // Output quad position directly to clip space
    gl_Position = vec4(aPos, 0.0, 1.0);
}
