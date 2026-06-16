#include "world.h"

namespace {
int chunkIndex(int x, int y, int z) {
    return x + World::SIZE * (y + World::SIZE * z);
}

bool isChunkPositionInBounds(const glm::ivec3& chunkPosition) {
    return chunkPosition.x >= 0 && chunkPosition.x < World::SIZE
        && chunkPosition.y >= 0 && chunkPosition.y < World::SIZE
        && chunkPosition.z >= 0 && chunkPosition.z < World::SIZE;
}
}

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

World::World() {
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            for (int z = 0; z < SIZE; z++) {
                getChunk(x, y, z) = Chunk(this, x, y, z);
            }
        }
    }

    for (Chunk& chunk : chunks)
        chunk.populateChunk();

    for (Chunk& chunk : chunks)
        chunk.buildChunkMesh();
}

Chunk& World::getChunk(int x, int y, int z) {
    return chunks[chunkIndex(x, y, z)];
}

const Chunk& World::getChunk(int x, int y, int z) const {
    return chunks[chunkIndex(x, y, z)];
}

Block* World::getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) {
    return const_cast<Block*>(static_cast<const World&>(*this).getBlock(chunkPosition, blockPosition));
}

const Block* World::getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) const {
    if (!isChunkPositionInBounds(chunkPosition))
        return nullptr;

    return getChunk(chunkPosition.x, chunkPosition.y, chunkPosition.z).getLocalBlock(blockPosition);
}
