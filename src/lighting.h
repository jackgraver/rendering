#ifndef LIGHTING_H
#define LIGHTING_H

#include "camera.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Lighting {
public:
    Lighting():
        shader("src/shaders/light_vert.glsl", "src/shaders/light_frag.glsl"),
        lightPos(0.0f) {
    }

    void drawLight(const glm::mat4& projection, const glm::mat4& view) {
        if (VAO == 0)
            glGenVertexArrays(1, &VAO);

        shader.use();

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        shader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    bool isValid() const {
        return shader.valid;
    }

    void cleanup() {
        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
    }

private:
    Shader shader;
    unsigned int VAO = 0;
    glm::vec3 lightPos;
};

#endif
