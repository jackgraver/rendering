#include "chunk.h"
#include "world.h"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

Chunk::Chunk(World* world, int x, int y, int z)
    : world(world)
    , chunkPos(x, y, z) {}

void Chunk::setWorld(World* world) {
    this->world = world;
}

void Chunk::populateChunk() {
    blocks.clear();

    for (unsigned x = 0; x < CHUNK_ROWS; x++) {
        for (unsigned y = 0; y < CHUNK_COLS; y++) {
            for (unsigned z = 0; z < CHUNK_HEIGHT; z++) {
                glm::ivec3 position(x, y, z);
                blocks.emplace(position, Block(DIRT));
            }
        }
    }

    meshDirty = true;
}

void Chunk::buildChunkMesh() {
    std::vector<float> vertices;

    glm::vec3 chunkOffset = {
        static_cast<float>(chunkPos.x * static_cast<int>(CHUNK_ROWS)),
        static_cast<float>(chunkPos.y * static_cast<int>(CHUNK_COLS)),
        static_cast<float>(chunkPos.z * static_cast<int>(CHUNK_HEIGHT))
    };

    for (const auto& blockEntry : blocks) {
        const glm::ivec3& localPos = blockEntry.first;
        const Block& block = blockEntry.second;

        if (block.type == AIR)
            continue;

        for (const FaceData& face : faces) {
            const Block* neighborBlock = getBlock(localPos + face.neighborOffset);
            bool shouldAddFace = neighborBlock == nullptr || neighborBlock->type == AIR;

            if (!shouldAddFace)
                continue;

            glm::vec3 base = chunkOffset + glm::vec3(localPos);
            for (int i = 0; i < 6; i++) {
                pushVertex(vertices, base + face.vertices[i], face.normal);
            }
        }
    }

    vertexCount = static_cast<int>(vertices.size() / 6);

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

void Chunk::drawChunk(Shader* shader) {
    if (meshDirty)
        buildChunkMesh();

    if (vertexCount == 0)
        return;

    shader->setVec3("objectColor", blockTypeColor(DIRT));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void Chunk::releaseGpuResources() {
    if (VAO != 0)
        glDeleteVertexArrays(1, &VAO);

    if (VBO != 0)
        glDeleteBuffers(1, &VBO);

    VAO = 0;
    VBO = 0;
}

bool Chunk::isLocalPositionInBounds(const glm::ivec3& localPos) const {
    return localPos.x >= 0 && localPos.x < static_cast<int>(CHUNK_ROWS)
        && localPos.y >= 0 && localPos.y < static_cast<int>(CHUNK_COLS)
        && localPos.z >= 0 && localPos.z < static_cast<int>(CHUNK_HEIGHT);
}

Block* Chunk::getLocalBlock(const glm::ivec3& localPos) {
    auto it = blocks.find(localPos);
    if (it == blocks.end())
        return nullptr;

    return &it->second;
}

const Block* Chunk::getLocalBlock(const glm::ivec3& localPos) const {
    auto it = blocks.find(localPos);
    if (it == blocks.end())
        return nullptr;

    return &it->second;
}

const Block* Chunk::getBlock(const glm::ivec3& localPos) const {
    if (isLocalPositionInBounds(localPos))
        return getLocalBlock(localPos);

    if (world == nullptr)
        return nullptr;

    glm::ivec3 worldBlockPos = {
        chunkPos.x * static_cast<int>(CHUNK_ROWS) + localPos.x,
        chunkPos.y * static_cast<int>(CHUNK_COLS) + localPos.y,
        chunkPos.z * static_cast<int>(CHUNK_HEIGHT) + localPos.z
    };

    glm::ivec3 neighborChunkPos = {
        floorDiv(worldBlockPos.x, static_cast<int>(CHUNK_ROWS)),
        floorDiv(worldBlockPos.y, static_cast<int>(CHUNK_COLS)),
        floorDiv(worldBlockPos.z, static_cast<int>(CHUNK_HEIGHT))
    };

    glm::ivec3 neighborLocalPos = {
        positiveMod(worldBlockPos.x, static_cast<int>(CHUNK_ROWS)),
        positiveMod(worldBlockPos.y, static_cast<int>(CHUNK_COLS)),
        positiveMod(worldBlockPos.z, static_cast<int>(CHUNK_HEIGHT))
    };

    return world->getBlock(neighborChunkPos, neighborLocalPos);
}
