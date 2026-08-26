#include "BlackHole.h"
#include <iostream>
#include <algorithm>

BlackHole::BlackHole() {
    position = glm::vec3(0.0f);
    mass = 1.0f;
}

BlackHole::~BlackHole() {
    ClearBodies();
}

void BlackHole::Initialize(glm::vec3 pos, float bhMass) {
    position = pos;
    mass = bhMass;
    bodies.clear();
}

void BlackHole::AddBody(glm::vec3 pos, glm::vec3 vel, float bodyMass, glm::vec3 color) {
    CelestialBody body;
    body.position = pos;
    body.velocity = vel;
    body.mass = bodyMass;
    body.color = color;
    body.isAlive = true;
    bodies.push_back(body);
}

void BlackHole::ClearBodies() {
    bodies.clear();
}

// Relativistic Paczynski-Wiita acceleration vector calculation
glm::vec3 BlackHole::CalculateAcceleration(const glm::vec3& bodyPos) {
    glm::vec3 rVec = bodyPos - position;
    float r = glm::length(rVec);
    float Rs = 2.0f * mass; // Schwarzschild Radius

    // Safety threshold near/inside the singularity
    if (r <= Rs * 1.01f) {
        return glm::vec3(0.0f);
    }

    // Paczynski-Wiita force magnitude: F = -G*M*m / (r - Rs)^2
    // Gravity approaches infinity as r approaches Rs, forcing unstable orbits to spiral in
    float denominator = (r - Rs) * (r - Rs);
    float accelMag = -mass / denominator;

    return (rVec / r) * accelMag;
}

// 4th-Order Runge-Kutta (RK4) Integrator
void BlackHole::Update(float deltaTime) {
    float Rs = 2.0f * mass;

    for (auto& body : bodies) {
        if (!body.isAlive) continue;

        glm::vec3 p0 = body.position;
        glm::vec3 v0 = body.velocity;

        // Step 1: k1
        glm::vec3 a1 = CalculateAcceleration(p0);
        glm::vec3 k1_v = a1 * deltaTime;
        glm::vec3 k1_p = v0 * deltaTime;

        // Step 2: k2
        glm::vec3 a2 = CalculateAcceleration(p0 + 0.5f * k1_p);
        glm::vec3 k2_v = a2 * deltaTime;
        glm::vec3 k2_p = (v0 + 0.5f * k1_v) * deltaTime;

        // Step 3: k3
        glm::vec3 a3 = CalculateAcceleration(p0 + 0.5f * k2_p);
        glm::vec3 k3_v = a3 * deltaTime;
        glm::vec3 k3_p = (v0 + 0.5f * k2_v) * deltaTime;

        // Step 4: k4
        glm::vec3 a4 = CalculateAcceleration(p0 + k3_p);
        glm::vec3 k4_v = a4 * deltaTime;
        glm::vec3 k4_p = (v0 + k3_v) * deltaTime;

        // Update position and velocity with weighted averages
        body.position += (k1_p + 2.0f * k2_p + 2.0f * k3_p + k4_p) / 6.0f;
        body.velocity += (k1_v + 2.0f * k2_v + 2.0f * k3_v + k4_v) / 6.0f;

        // Thin Accretion Disk Drag/Friction (Simulates ISCO disintegration)
        float currentRadius = glm::length(body.position - position);
        if (currentRadius < 3.0f * Rs && currentRadius > Rs) { // ISCO boundary
            body.velocity *= (1.0f - 0.2f * deltaTime); // Drag forces body to spiral in faster
        }

        // Event Horizon Capture Check
        if (currentRadius <= Rs) {
            body.isAlive = false; // Consumed by the black hole
        }
    }

    // Remove consumed celestial bodies
    bodies.erase(
        std::remove_if(bodies.begin(), bodies.end(), [](const CelestialBody& b) { return !b.isAlive; }),
        bodies.end()
    );
}