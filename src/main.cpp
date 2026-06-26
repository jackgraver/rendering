#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "lighting.h"
#include "shader.h"
#include "camera.h"

#include <iostream>
#include <cstdlib>
#include <iomanip>
#include "world.h"

GLFWwindow* initialize();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(10.0f, 15.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(5.2f, 100.0f, 8.0f);

bool renderPolygons = true;
bool pKeyWasPressed = false;

std::unique_ptr<World> world;

namespace {
constexpr double kSlowFrameMs = 20.0;
}

int main() {
    GLFWwindow* window = initialize();
    if (window == nullptr) {
        return 0;
    }

    Lighting light;

    world = std::make_unique<World>();
    world->requestChunks(camera.Position);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    std::cout << "-----------------------" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        world->worldShader.use();
        world->worldShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        world->worldShader.setVec3("lightPos", lightPos);
        world->worldShader.setVec3("viewPos", camera.Position);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 300.0f);
        glm::mat4 view = camera.GetViewMatrix();
        world->worldShader.setMat4("projection", projection);
        world->worldShader.setMat4("view", view);

        const double worldUpdateStart = glfwGetTime();
        world->requestChunks(camera.Position);
        const double worldUpdateEnd = glfwGetTime();

        world->drawChunks();
        const double worldDrawEnd = glfwGetTime();

        const double worldUpdateMs = (worldUpdateEnd - worldUpdateStart) * 1000.0;
        const double worldDrawMs = (worldDrawEnd - worldUpdateEnd) * 1000.0;
        const double worldFrameMs = (worldDrawEnd - worldUpdateStart) * 1000.0;

        if (worldFrameMs >= kSlowFrameMs) {
            std::cout << std::fixed << std::setprecision(2)
                      << "[frame] world update=" << worldUpdateMs << "ms"
                      << " draw=" << worldDrawMs << "ms"
                      << " total=" << worldFrameMs << "ms" << std::endl;
        }

        light.drawLight(projection, view);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // break;
    }

    light.cleanup();

    for (auto& chunk : world->chunks) {
        chunk.second.releaseGpuResources();
    }

    glfwTerminate();
    return 0;
}

GLFWwindow* initialize() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    return window;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    const int pKeyState = glfwGetKey(window, GLFW_KEY_P);
    if (pKeyState == GLFW_PRESS && !pKeyWasPressed) {
        renderPolygons = !renderPolygons;
        glPolygonMode(GL_FRONT_AND_BACK, renderPolygons ? GL_FILL : GL_LINE);
    }
    pKeyWasPressed = (pKeyState == GLFW_PRESS);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}



// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
