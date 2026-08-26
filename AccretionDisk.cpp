#include "AccretionDisk.h"
#include <iostream>
#include <algorithm>

AccretionDisk::AccretionDisk() {
    VAO = 0;
    VBO = 0;
}

AccretionDisk::~AccretionDisk() {
    Shutdown();
}

bool AccretionDisk::Initialize() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Reserve GPU memory for up to MAX_PARTICLES
    // Attributes: Position (3 floats), Color (3 floats) -> 6 floats per vertex
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Position Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color Attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void AccretionDisk::EmitDebris(const glm::vec3& pos, const glm::vec3& vel, const glm::vec3& color, int count) {
    for (int i = 0; i < count; ++i) {
        if (particles.size() >= MAX_PARTICLES) break;

        DebrisParticle p;
        p.position = pos;

        // Add random dispersion velocity to simulate tidal disruption
        float spreadX = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;
        float spreadY = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;
        float spreadZ = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;

        p.velocity = vel + glm::vec3(spreadX, spreadY, spreadZ);
        p.color = color;
        p.life = 1.0f; // Full lifetime

        particles.push_back(p);
    }
}

void AccretionDisk::Update(float deltaTime, float bhMass) {
    float Rs = 2.0f * bhMass;

    for (auto& p : particles) {
        p.life -= deltaTime * 0.5f; // Gradual decay

        // Gravitational force pulling particles inward
        glm::vec3 rVec = -p.position;
        float r = glm::length(rVec);

        if (r > 0.001f) {
            glm::vec3 accel = (rVec / r) * (bhMass / (r * r));
            p.velocity += accel * deltaTime;
        }

        p.position += p.velocity * deltaTime;

        // Kill particles passing into event horizon or when lifetime ends
        if (r <= Rs || p.life <= 0.0f) {
            p.life = 0.0f;
        }
    }

    // Remove dead particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](const DebrisParticle& p) { return p.life <= 0.0f; }),
        particles.end()
    );
}

void AccretionDisk::Draw(const Shader& particleShader, const glm::mat4& view, const glm::mat4& projection) {
    if (particles.empty()) return;

    // Pack vertex data: [X, Y, Z, R, G, B] per particle
    std::vector<float> vertexData;
    vertexData.reserve(particles.size() * 6);

    for (const auto& p : particles) {
        // Fade particle color with remaining lifetime
        glm::vec3 fadedColor = p.color * p.life;

        vertexData.push_back(p.position.x);
        vertexData.push_back(p.position.y);
        vertexData.push_back(p.position.z);
        vertexData.push_back(fadedColor.r);
        vertexData.push_back(fadedColor.g);
        vertexData.push_back(fadedColor.b);
    }

    // Upload dynamic buffer to GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());

    // Setup Shader & Draw
    particleShader.Use();
    particleShader.setMat4("u_view", view);
    particleShader.setMat4("u_projection", projection);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles.size()));
    glBindVertexArray(0);
}

void AccretionDisk::Shutdown() {
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    particles.clear();
}