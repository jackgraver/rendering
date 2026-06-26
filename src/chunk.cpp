#include "chunk.h"
#include "world.h"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "perlin.h"

Chunk::Chunk(World* world, int x, int y, int z)
    : world(world)
    , chunkPos(x, y, z) {}

void Chunk::setWorld(World* world) {
    this->world = world;
}

void Chunk::populateChunk() {
    blocks.clear();

    constexpr float baseHeightWorld = 2.4f;
    constexpr float broadHillFrequency = 0.08f;
    constexpr float broadHillAmplitude = 1.6f;
    constexpr float detailFrequency = 0.35f;
    constexpr float detailAmplitude = 0.35f;

    for (unsigned x = 0; x < CHUNK_WIDTH; x++) {
        for (unsigned z = 0; z < CHUNK_DEPTH; z++) {
            const float worldX = (static_cast<float>(chunkPos.x * static_cast<int>(CHUNK_WIDTH)) + static_cast<float>(x)) * BLOCK_SIZE;
            const float worldZ = (static_cast<float>(chunkPos.z * static_cast<int>(CHUNK_DEPTH)) + static_cast<float>(z)) * BLOCK_SIZE;

            const float broadNoise = perlin(worldX * broadHillFrequency, worldZ * broadHillFrequency);

            float detailNoise = 0.0f;
            float amplitude = 1.0f;
            float frequency = detailFrequency;
            float maxAmplitude = 0.0f;

            for (int octave = 0; octave < 4; octave++) {
                detailNoise += perlin(worldX * frequency, worldZ * frequency) * amplitude;
                maxAmplitude += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            detailNoise /= maxAmplitude; // roughly [-1, 1]

            const float terrainHeightWorld = baseHeightWorld
                + (broadNoise * broadHillAmplitude)
                + (detailNoise * detailAmplitude);

            for (unsigned y = 0; y < CHUNK_HEIGHT; y++) {
                const int worldBlockY = chunkPos.y * static_cast<int>(CHUNK_HEIGHT) + static_cast<int>(y);
                const float worldY = static_cast<float>(worldBlockY) * BLOCK_SIZE;

                if (worldY <= terrainHeightWorld) {
                    blocks.emplace(glm::ivec3(x, y, z), Block(DIRT));
                }
            }
        }
    }
}

void Chunk::buildChunkMesh() {
    setMeshVertices(buildMeshVertices());
}

std::vector<float> Chunk::buildMeshVertices() const {
    std::vector<float> vertices;

    glm::vec3 chunkOffset = {
        static_cast<float>(chunkPos.x) * CHUNK_WORLD_WIDTH,
        static_cast<float>(chunkPos.y) * CHUNK_WORLD_HEIGHT,
        static_cast<float>(chunkPos.z) * CHUNK_WORLD_DEPTH
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

            glm::vec3 base = chunkOffset + (glm::vec3(localPos) * BLOCK_SIZE);
            for (int i = 0; i < 6; i++) {
                pushVertex(vertices, base + (face.vertices[i] * BLOCK_SIZE), face.normal);
            }
        }
    }

    return vertices;
}

void Chunk::setMeshVertices(std::vector<float>&& vertices) {
    meshVertices = std::move(vertices);
    vertexCount = static_cast<int>(meshVertices.size() / 6);
    gpuUploadDirty = true;
}

void Chunk::uploadMesh() {
    if (!gpuUploadDirty)
        return;

    if (chunkVAO == 0)
        glGenVertexArrays(1, &chunkVAO);

    if (chunkVBO == 0)
        glGenBuffers(1, &chunkVBO);

    glBindVertexArray(chunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        meshVertices.size() * sizeof(float),
        meshVertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    gpuUploadDirty = false;
}

void Chunk::drawChunk(Shader* shader) {
    if (gpuUploadDirty || vertexCount == 0 || chunkVAO == 0)
        return;

    shader->setVec3("objectColor", blockTypeColor(DIRT));

    glBindVertexArray(chunkVAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void Chunk::releaseGpuResources() {
    if (chunkVAO != 0)
        glDeleteVertexArrays(1, &chunkVAO);

    if (chunkVBO != 0)
        glDeleteBuffers(1, &chunkVBO);

    chunkVAO = 0;
    chunkVBO = 0;
}

bool Chunk::isLocalPositionInBounds(const glm::ivec3& localPos) const {
    return localPos.x >= 0 && localPos.x < static_cast<int>(CHUNK_WIDTH)
        && localPos.y >= 0 && localPos.y < static_cast<int>(CHUNK_HEIGHT)
        && localPos.z >= 0 && localPos.z < static_cast<int>(CHUNK_DEPTH);
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
        chunkPos.x * CHUNK_WIDTH + localPos.x,
        chunkPos.y * CHUNK_HEIGHT + localPos.y,
        chunkPos.z * CHUNK_DEPTH + localPos.z
    };

    glm::ivec3 neighborChunkPos = {
        floorDiv(worldBlockPos.x, CHUNK_WIDTH),
        floorDiv(worldBlockPos.y, CHUNK_HEIGHT),
        floorDiv(worldBlockPos.z, CHUNK_DEPTH)
    };

    glm::ivec3 neighborLocalPos = {
        positiveMod(worldBlockPos.x, CHUNK_WIDTH),
        positiveMod(worldBlockPos.y, CHUNK_HEIGHT),
        positiveMod(worldBlockPos.z, CHUNK_DEPTH)
    };

    return world->getBlock(neighborChunkPos, neighborLocalPos);
}
