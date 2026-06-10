#include <iostream>
// Library that helps with avoid repetitive boiler plate for different driver support on graphics cards
#include <glad/glad.h>
// Library for OpenGL, bare necessities required for rendeirng to screen.
//  Helps create OpenGL context, define window parameters and handle user input
#include <GLFW/glfw3.h>


#include "main.h"

// Set of 3D points to form a triangle
// All values (x, y, z) between -1 and 1
float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

// Simplest vertex shader
// No processing just forwards input data to shader output
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

// Fragment shader
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";

int main()
{
    glfwInit();
    // Configuring glfw, first param specifies what we are configuring & 2nd is value
    // - Options from large enum all prefixed by GLFW_
    // - All options - https://www.glfw.org/docs/latest/window.html#window_hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Tell using major OpenGL version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Tell using minor OpenGL version 3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Tell using Core-profile
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Window object, width and height specified first
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD before any OpenGL function calls
    // - Pass function to load address of OpenGL function pointers (OS-specific)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Size of rendering window, specifies region inside window OpenGL should render to
    glViewport(0, 0, 800, 600);

    // Resize handler
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Vertex buffer object, OpenGL object with unique ID
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    // Bind to vertex buffer type
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Copies vertex data into buffer memory
    // - 1st - Buffer to copy to
    // - 2nd - Size of data
    // - 3rd Actual data
    // - How graphics card should manage data
    // -- GL_STREAM_DRAW: the data is set only once and used by the GPU at most a few times.
    // -- GL_STATIC_DRAW: the data is set only once and used many times.
    // -- GL_DYNAMIC_DRAW: the data is changed a lot and used many times. (places in memory that allows faster writes)
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create OpenGL Shader Object, referenced by ID
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER); // Use GL_VERTEX_SHADER to specify vertex
    // Attach shader source code to shader object
    // - 1st - Shader object to compile to
    // - 2nd - How many strings passing as source code
    // - 3rd - Source code
    // - 4th - Just null for now
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader); // and compile

    // Fragment shader setup, same as vertex
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);

    // Shader program object
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    // Attach compiled shaders to program object and link them
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Clean up shader objects as dont need after linked to program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Need to detail vertex data structure
    // Vertex buffer data is 3 32-bit (4 byte) floating point values, with 3 vertices defined
    // which are packed together tightly in the array
    // 1st - Which vertex attribute we want to configure. In vertex shader we specified
    //       the location of the position vertex with layout (location = 0)
    // 2nd - Size of vertex attribute (vec3 so 3 values)
    // 3rd - Type of data which is float
    // 4th - Specify if we want data to be normalized
    // 5th - Stride, space between consecutive vertex attributes. So 3 times the size of float
    // 6th - Offset, required type is void* so need to cast
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // Enable vertex attribute with vertex attribute location as arg. Vertex attributes disabled by default
    glEnableVertexAttribArray(0);


    // Create Vertex Array Object
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Render loop
    while(!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swaps color buffer that is used to render this render iteration and show it as
        // output to the screen
        glfwSwapBuffers(window);
        // checks if any events are triggered (key or mouse input), updates windows state and
        // calls corresponding functions (which we can register via callback methods)
        glfwPollEvents();
    }
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
