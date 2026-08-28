#pragma once

#include <glm/glm.hpp>
#include <vector>

// Structure representing orbiting bodies (stars/planets)
struct CelestialBody {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    glm::vec3 color;
    bool isAlive = true;
};

class BlackHole
{
public:
    BlackHole();
    ~BlackHole();

    void Initialize(glm::vec3 pos = glm::vec3(0.0f), float mass = 1.0f);

    // Updates celestial body physics and spiraling trajectory (RK4 Integration)
    void Update(float deltaTime);

    // Dynamic Body Controls
    void AddBody(glm::vec3 pos, glm::vec3 vel, float bodyMass, glm::vec3 color);
    void ClearBodies();

    // Getters
    glm::vec3 GetPosition() const { return position; }
    float GetMass() const { return mass; }
    float GetSchwarzschildRadius() const { return 2.0f * mass; }
    const std::vector<CelestialBody>& GetBodies() const { return bodies; }

    // Setters
    void SetMass(float newMass) { mass = newMass; }

private:
    glm::vec3 position;
    float mass; // Black hole mass

    std::vector<CelestialBody> bodies;

    // Calculates gravitational acceleration on a body using Paczynski-Wiita potential
    glm::vec3 CalculateAcceleration(const glm::vec3& bodyPos);
};