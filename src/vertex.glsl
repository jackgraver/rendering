#version 330 core
layout(location = 0) in vec3 aPos;

out vec4 vertexColor; // need color output to fragment shader

void main()
{
    gl_Position = vec4(aPOs, 1.0); // Pass a vec3 + variable to vec4 constructor
    vertexColor = vec4(0.5, 0.0, 0.0, 1.0); // Set output to dark-red color
}
