#include "Renderer.h"
#include <iostream>

Renderer::Renderer() {
    quadVAO = 0;
    quadVBO = 0;
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize() {
    SetupScreenQuad();
    return true;
}

void Renderer::SetupScreenQuad() {
    // Standard full-screen quad (2 triangles covering clip space)
    float quadVertices[] = {
        // Position (X, Y)
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position Attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::Render(const Shader& shader, const glm::vec3& camPos, const glm::vec3& camFront,
    const glm::vec3& camUp, const glm::vec2& resolution, float time, float mass) {
    shader.Use();

    // 1. Calculate Camera Orientation Vectors
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    glm::vec3 up = glm::normalize(glm::cross(right, camFront));

    // 2. Dispatch Simulation Uniforms to Shader
    shader.setVec2("u_resolution", resolution);
    shader.setFloat("u_time", time);
    shader.setFloat("u_mass", mass);

    // Camera Uniforms
    shader.setVec3("u_camPos", camPos);
    shader.setVec3("u_camForward", camFront);
    shader.setVec3("u_camUp", up);
    shader.setVec3("u_camRight", right);

    // 3. Draw Fullscreen Quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::Shutdown() {
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
}