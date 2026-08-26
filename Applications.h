#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
class Applications
{
public:
    Applications();
    ~Applications();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    // Window Management
    GLFWwindow* window = nullptr;
    unsigned int SCR_WIDTH = 1280;
    unsigned int SCR_HEIGHT = 720;

    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Orbital Camera Variables
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    float yaw = -90.0f;
    float pitch = 5.0f;
    float distance = 25.0f; // Zoom distance (Supports deep zoom-out to 120.0f+)

    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;
    bool firstMouse = true;

    // Simulation Parameters
    float blackHoleMass = 1.0f;

    // Screen Quad OpenGL Handles
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    Shader* shader = nullptr;

    // Helper Methods
    void ProcessInput(GLFWwindow* window);
    void UpdateCameraVectors();
    void SetupScreenQuad();
    bool CompileShaders();

    // GLFW Static Callback Wrappers
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
