#ifndef WORLD_H
#define WORLD_H

#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// MUST be here (used in Chunk layout)
constexpr unsigned CHUNK_COLS = 2;
constexpr unsigned CHUNK_ROWS = 8;
constexpr unsigned CHUNK_HEIGHT = 8;
constexpr unsigned CHUNK_SIZE = CHUNK_ROWS * CHUNK_COLS * CHUNK_HEIGHT;

enum BLOCK_TYPE {
    AIR,
    DIRT,
    STONE,
};

struct Block {
    BLOCK_TYPE type;
};

inline glm::vec3 indexToLocal(int i) {
    int x = i % CHUNK_ROWS;
    int y = (i / CHUNK_ROWS) % CHUNK_COLS;
    int z = i / (CHUNK_ROWS * CHUNK_COLS);

    return glm::vec3(x, y, z);
}

class Chunk {
public:
    Chunk() = default;
    Chunk(int x, int y, int z)
        : chunkPos(x, y, z)
    {
        populateChunk();
    }

    void populateChunk() {
        for (unsigned x = 0; x < CHUNK_ROWS; x++) {
            for (unsigned y = 0; y < CHUNK_COLS; y++) {
                for (unsigned z = 0; z < CHUNK_HEIGHT; z++) {

                    unsigned index = x + CHUNK_ROWS * (y + CHUNK_COLS * z);

                    Block& b = blocks[index];

                    b.type = (rand() % 100 > 50) ? AIR : DIRT;
                }
            }
        }
    }
    void drawChunk(Shader* shader) {
        glm::vec3 chunkOffset = {
            this->chunkPos.x * CHUNK_ROWS,
            this->chunkPos.y * CHUNK_COLS,
            this->chunkPos.z * CHUNK_HEIGHT
        };

        for (int i = 0; i < CHUNK_SIZE; i++)
        {
            if (this->blocks[i].type == AIR)
                continue;

            glm::vec3 localPos = indexToLocal(i);
            glm::vec3 worldPos = chunkOffset + localPos;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
            shader->setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glm::ivec3 chunkPos;
    Block blocks[CHUNK_SIZE];
};

class World {
public:
    static constexpr int SIZE = 4;

    Chunk chunks[SIZE * SIZE * SIZE];

    Chunk& getChunk(int x, int y, int z)
    {
        int index = x + SIZE * (y + SIZE * z);
        return chunks[index];
    }
};

#endif
