#version 330 core
in vec4 vertexColor; // same name and type as vertex output

out vec4 FragColor;

void main()
{
    FragColor = vertexColor;
}
