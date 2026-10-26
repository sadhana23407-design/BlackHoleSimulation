#include "Applications.h"
#include <iostream>

Applications::Applications() {}

Applications::~Applications() {
    Shutdown();
}

bool Applications::Initialize() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Black Hole Simulation - General Relativity", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW Window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return false;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Load Shaders
    shader = new Shader("blackhole.vert.glsl", "fragment.glsl");

    SetupScreenQuad();
    UpdateCameraVectors();

    return true;
}

void Applications::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, worldUp));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

void Applications::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader->Use();

        // Pass Core Camera & Raymarching Uniforms
        shader->setVec2("u_resolution", glm::vec2(static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)));
        shader->setFloat("u_time", currentFrame);
        shader->setFloat("u_mass", blackHoleMass);

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

        shader->setVec3("u_camPos", cameraPos);
        shader->setVec3("u_camForward", cameraFront);
        shader->setVec3("u_camUp", cameraUp);
        shader->setVec3("u_camRight", right);

        // Draw Screen-Filling Quad
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Applications::ProcessInput(GLFWwindow* win) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    float velocity = cameraSpeed * deltaTime;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * velocity;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * velocity;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;

    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
}

void Applications::SetupScreenQuad() {
    float quadVertices[] = {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void Applications::FramebufferSizeCallback(GLFWwindow* win, int width, int height) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
    if (!app) return;

    app->SCR_WIDTH = width;
    app->SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void Applications::MouseCallback(GLFWwindow* win, double xposIn, double yposIn) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
    if (!app) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->firstMouse) {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos;
    app->lastX = xpos;
    app->lastY = ypos;

    float sensitivity = 0.1f;
    app->yaw += xoffset * sensitivity;
    app->pitch += yoffset * sensitivity;

    if (app->pitch > 89.0f)  app->pitch = 89.0f;
    if (app->pitch < -89.0f) app->pitch = -89.0f;

    app->UpdateCameraVectors();
}

void Applications::ScrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
    if (!app) return;

    app->cameraSpeed += static_cast<float>(yoffset) * 2.5f;
    app->cameraSpeed = glm::clamp(app->cameraSpeed, 2.0f, 150.0f);
}

void Applications::KeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
    if (!app) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);
}

void Applications::Shutdown() {
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);

    if (shader) {
        delete shader;
        shader = nullptr;
    }

    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}