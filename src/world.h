#ifndef WORLD_H
#define WORLD_H

#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>

#include <unordered_map>
#include <utility>
#include <vector>

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

inline glm::vec3 blockTypeColor(BLOCK_TYPE type) {
    switch (type) {
        case AIR:
            return glm::vec3(1.0f, 1.0f, 1.0f);
        case DIRT:
            return glm::vec3(0.39f, 0.25f, 0.09f);
        case STONE:
            return glm::vec3(0.66f, 0.66f, 0.66f);
    }
    return glm::vec3(1.0f, 1.0f, 1.0f);
}

struct Block {
    Block() = default;
    Block(BLOCK_TYPE type)
        : type(type)
    {
    }

    BLOCK_TYPE type = AIR;
};

struct IVec3Hash {
    std::size_t operator()(const glm::ivec3& v) const {
        std::size_t x = std::hash<int>{}(v.x);
        std::size_t y = std::hash<int>{}(v.y);
        std::size_t z = std::hash<int>{}(v.z);

        return x ^ (y << 1) ^ (z << 2);
    }
};

inline glm::ivec3 indexToLocal(int i) {
    int x = i % CHUNK_ROWS;
    int y = (i / CHUNK_ROWS) % CHUNK_COLS;
    int z = i / (CHUNK_ROWS * CHUNK_COLS);

    return glm::ivec3(x, y, z);
}

struct FaceData {
    glm::ivec3 neighborOffset;
    glm::vec3 normal;
    glm::vec3 vertices[6];
};


const FaceData faces[6] = {
    // +X
    {{ 1, 0, 0}, { 1.0f, 0.0f, 0.0f}, {
        {1, 0, 0}, {1, 0, 1}, {1, 1, 1},
        {1, 0, 0}, {1, 1, 1}, {1, 1, 0},
    }},
    // -X
    {{-1, 0, 0}, {-1.0f, 0.0f, 0.0f}, {
        {0, 0, 0}, {0, 1, 1}, {0, 0, 1},
        {0, 0, 0}, {0, 1, 0}, {0, 1, 1},
    }},
    // +Y
    {{ 0, 1, 0}, { 0.0f, 1.0f, 0.0f}, {
        {0, 1, 0}, {1, 1, 1}, {0, 1, 1},
        {0, 1, 0}, {1, 1, 0}, {1, 1, 1},
    }},
    // -Y
    {{ 0,-1, 0}, { 0.0f,-1.0f, 0.0f}, {
        {0, 0, 0}, {0, 0, 1}, {1, 0, 1},
        {0, 0, 0}, {1, 0, 1}, {1, 0, 0},
    }},
    // +Z
    {{ 0, 0, 1}, { 0.0f, 0.0f, 1.0f}, {
        {0, 0, 1}, {0, 1, 1}, {1, 1, 1},
        {0, 0, 1}, {1, 1, 1}, {1, 0, 1},
    }},
    // -Z
    {{ 0, 0,-1}, { 0.0f, 0.0f,-1.0f}, {
        {0, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0},
    }},
};

inline void pushVertex(std::vector<float>& vertices, const glm::vec3& position, const glm::vec3& normal) {
    vertices.push_back(position.x);
    vertices.push_back(position.y);
    vertices.push_back(position.z);
    vertices.push_back(normal.x);
    vertices.push_back(normal.y);
    vertices.push_back(normal.z);
}

class Chunk {
public:
    Chunk() = default;
    Chunk(int x, int y, int z): chunkPos(x, y, z) {
        populateChunk();
    }

    void populateChunk() {
        for (unsigned x = 0; x < CHUNK_ROWS; x++) {
            for (unsigned y = 0; y < CHUNK_COLS; y++) {
                for (unsigned z = 0; z < CHUNK_HEIGHT; z++) {
                    glm::ivec3 position(x, y, z);

                    blocks.emplace(position, Block(DIRT));
                }
            }
        }
        buildChunkMesh();
    }

    void buildChunkMesh() {
        std::vector<float> vertices;

        glm::vec3 chunkOffset = {
            this->chunkPos.x * CHUNK_ROWS,
            this->chunkPos.y * CHUNK_COLS,
            this->chunkPos.z * CHUNK_HEIGHT
        };

        for (const auto& blockEntry : this->blocks) {
            const glm::ivec3& localPos = blockEntry.first;
            const Block& block = blockEntry.second;

            if (block.type == AIR)
                continue;

            for (const FaceData& face : faces) {
                glm::ivec3 neighborPos = localPos + face.neighborOffset;
                auto neighborBlock = this->blocks.find(neighborPos);

                bool shouldAddFace = neighborBlock == this->blocks.end()
                                    || neighborBlock->second.type == AIR;
                if (!shouldAddFace)
                    continue;

                glm::vec3 base = chunkOffset + glm::vec3(localPos);
                for (int i = 0; i < 6; i++) {
                    pushVertex(vertices, base + face.vertices[i], face.normal);
                }
            }
        }

        vertexCount = vertices.size() / 6;

        if (VAO == 0)
            glGenVertexArrays(1, &VAO);

        if (VBO == 0)
            glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(float),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        meshDirty = false;
    }

    void drawChunk(Shader* shader) {
        if (meshDirty)
            buildChunkMesh();

        if (vertexCount == 0)
            return;

        shader->setVec3("objectColor", blockTypeColor(DIRT));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void releaseGpuResources() {
        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);

        if (VBO != 0)
            glDeleteBuffers(1, &VBO);

        VAO = 0;
        VBO = 0;
    }

// private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    bool meshDirty = true;
    int vertexCount = 0;
    glm::ivec3 chunkPos;
    // Map of local block positions to blocks in the chunk
    std::unordered_map<glm::ivec3, Block, IVec3Hash> blocks;
};

class World {
public:
    static constexpr int SIZE = 2;

    Chunk chunks[SIZE * SIZE * SIZE];

    Chunk& getChunk(int x, int y, int z)
    {
        int index = x + SIZE * (y + SIZE * z);
        return chunks[index];
    }

    Block* getBlock(glm::ivec3 chunkPosition, glm::ivec3 blockPosition) {
        int index = chunkPosition.x + SIZE * (chunkPosition.y + SIZE * chunkPosition.z);
        return &chunks[index].blocks[blockPosition];
    }
};

#endif
