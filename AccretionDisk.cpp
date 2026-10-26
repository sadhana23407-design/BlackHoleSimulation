#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

// Struct for dynamic debris particles stripped from spiraling bodies
struct DebrisParticle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float life; // Remaining lifetime (0.0 to 1.0)
};

class AccretionDisk
{
public:
    AccretionDisk();
    ~AccretionDisk();

    bool Initialize();
    void Update(float deltaTime, float bhMass);
    void Draw(const Shader& particleShader, const glm::mat4& view, const glm::mat4& projection);
    void EmitDebris(const glm::vec3& pos, const glm::vec3& vel, const glm::vec3& color, int count);
    void Shutdown();

private:
    std::vector<DebrisParticle> particles;

    unsigned int VAO;
    unsigned int VBO;

    const int MAX_PARTICLES = 10000;
};