#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Render(const Shader& shader, const glm::vec3& camPos, const glm::vec3& camFront,
        const glm::vec3& camUp, const glm::vec2& resolution, float time, float mass);
    void Shutdown();

private:
    unsigned int quadVAO;
    unsigned int quadVBO;

    void SetupScreenQuad();
};