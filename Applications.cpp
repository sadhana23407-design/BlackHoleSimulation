#include "Applications.h"
#include <iostream>
#include <vector>

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

    // Register Callbacks & Mouse Capture
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

    // Load and compile shaders from GLSL files
    shader = new Shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    SetupScreenQuad();
    UpdateCameraVectors();

    return true;
}

void Applications::UpdateCameraVectors() {
    // Calculate spherical camera position orbiting black hole center (0,0,0)
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);

    cameraPos.x = distance * cos(radPitch) * cos(radYaw);
    cameraPos.y = distance * sin(radPitch);
    cameraPos.z = distance * cos(radPitch) * sin(radYaw);

    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw + 180.0f));
    front.y = -sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw + 180.0f));
    cameraFront = glm::normalize(front);

    // 3. Recalculate up and right vectors
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
        UpdateCameraVectors();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Activate shader program via Shader class
        shader->Use();

        // Pass uniforms via Shader utility setters
        shader->setVec2("u_resolution", glm::vec2(static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)));
        shader->setFloat("u_time", currentFrame);
        shader->setFloat("u_mass", blackHoleMass);

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

        shader->setVec3("u_camPos", cameraPos);
        shader->setVec3("u_camForward", cameraFront);
        shader->setVec3("u_camUp", cameraUp);
        shader->setVec3("u_camRight", right);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Applications::ProcessInput(GLFWwindow* win) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // Zoom shortcuts
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
        distance = glm::clamp(distance - 10.0f * deltaTime, 4.0f, 120.0f);
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
        distance = glm::clamp(distance + 10.0f * deltaTime, 4.0f, 120.0f);
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
    app->SCR_WIDTH = width;
    app->SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void Applications::MouseCallback(GLFWwindow* win, double xposIn, double yposIn) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
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

    float sensitivity = 0.15f;
    app->yaw += xoffset * sensitivity;
    app->pitch += yoffset * sensitivity;

    // Prevent orbital flip
    if (app->pitch > 89.0f) app->pitch = 89.0f;
    if (app->pitch < -89.0f) app->pitch = -89.0f;
}

void Applications::ScrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    auto* app = reinterpret_cast<Applications*>(glfwGetWindowUserPointer(win));
    app->distance -= static_cast<float>(yoffset) * 1.5f;
    app->distance = glm::clamp(app->distance, 4.0f, 120.0f);
}

void Applications::KeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
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
