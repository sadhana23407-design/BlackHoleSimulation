#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class Applications {
public:
    Applications();
    ~Applications();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    GLFWwindow* window = nullptr;
    int SCR_WIDTH = 1280;
    int SCR_HEIGHT = 720;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Camera Navigation Settings
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 25.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float cameraSpeed = 15.0f;

    float lastX = 1280.0f / 2.0f;
    float lastY = 720.0f / 2.0f;
    bool firstMouse = true;

    // Black Hole Parameter
    float blackHoleMass = 2.5f;

    // Screen Quad Render Data
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    Shader* shader = nullptr;

    void ProcessInput(GLFWwindow* window);
    void UpdateCameraVectors();
    void SetupScreenQuad();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};