#include "world.h"

#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace {
constexpr int kWorldChunkHeight = 4;

bool isChunkPositionInBounds(const glm::ivec3& chunkPosition) {
    return chunkPosition.y >= 0 && chunkPosition.y < kWorldChunkHeight;
}
}

// One entry per block face used by chunk mesh generation.
// Each FaceData stores:
// - the neighbor block offset to test for occlusion
// - the face normal for lighting
// - the 6 vertices (2 triangles) for that face in local block space
const FaceData faces[6] = {
    // +X
    {{ 1, 0, 0}, { 1.0f, 0.0f, 0.0f}, {
        {1, 0, 0}, {1, 1, 1}, {1, 0, 1},
        {1, 0, 0}, {1, 1, 0}, {1, 1, 1},
    }},
    // -X
    {{-1, 0, 0}, {-1.0f, 0.0f, 0.0f}, {
        {0, 0, 0}, {0, 0, 1}, {0, 1, 1},
        {0, 0, 0}, {0, 1, 1}, {0, 1, 0},
    }},
    // +Y
    {{ 0, 1, 0}, { 0.0f, 1.0f, 0.0f}, {
        {0, 1, 0}, {0, 1, 1}, {1, 1, 1},
        {0, 1, 0}, {1, 1, 1}, {1, 1, 0},
    }},
    // -Y
    {{ 0,-1, 0}, { 0.0f,-1.0f, 0.0f}, {
        {0, 0, 0}, {1, 0, 1}, {0, 0, 1},
        {0, 0, 0}, {1, 0, 0}, {1, 0, 1},
    }},
    // +Z
    {{ 0, 0, 1}, { 0.0f, 0.0f, 1.0f}, {
        {0, 0, 1}, {1, 1, 1}, {0, 1, 1},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1},
    }},
    // -Z
    {{ 0, 0,-1}, { 0.0f, 0.0f,-1.0f}, {
        {0, 0, 0}, {0, 1, 0}, {1, 1, 0},
        {0, 0, 0}, {1, 1, 0}, {1, 0, 0},
    }},
};

World::World() {
    centerChunk = Coords(0, 0, 0);
    chunks.reserve(LOAD_RADIUS * LOAD_RADIUS * LOAD_RADIUS);
}

void World::updateLoadedChunks() {
    const int minX = centerChunk.x() - LOAD_RADIUS;
    const int maxX = centerChunk.x() + LOAD_RADIUS;
    const int minY = 0;
    const int maxY = kWorldChunkHeight - 1;
    const int minZ = centerChunk.z() - LOAD_RADIUS;
    const int maxZ = centerChunk.z() + LOAD_RADIUS;

    for (auto it = chunks.begin(); it != chunks.end(); ) {
        Chunk& chunk = it->second;

        if (chunk.chunkPos.x > maxX || chunk.chunkPos.x < minX ||
            chunk.chunkPos.y > maxY || chunk.chunkPos.y < minY ||
            chunk.chunkPos.z > maxZ || chunk.chunkPos.z < minZ) {
            chunk.releaseGpuResources();
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }

    std::vector<Coords> createdChunks;
    createdChunks.reserve((maxX - minX + 1) * (maxY - minY + 1) * (maxZ - minZ + 1));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                const Coords chunkCoords(x, y, z);
                if (chunks.find(chunkCoords) != chunks.end())
                    continue;

                chunks.emplace(chunkCoords, Chunk(this, x, y, z));
                createdChunks.push_back(chunkCoords);
            }
        }
    }

    if (createdChunks.empty())
        return;

    for (const Coords& coords : createdChunks)
        chunks.at(coords).populateChunk();

    std::unordered_set<Coords> chunksToRebuild;
    for (const Coords& coords : createdChunks) {
        chunksToRebuild.insert(coords);

        for (const FaceData& face : faces) {
            const Coords neighborCoords = coords + face.neighborOffset;
            if (chunks.find(neighborCoords) != chunks.end())
                chunksToRebuild.insert(neighborCoords);
        }
    }

    for (const Coords& coords : chunksToRebuild)
        chunks.at(coords).buildChunkMesh();
}

Chunk& World::getChunk(int x, int y, int z) {
    const Coords chunkCoords(x, y, z);
    auto result = chunks.try_emplace(chunkCoords, this, x, y, z);
    return result.first->second;
}

const Chunk& World::getChunk(int x, int y, int z) const {
    auto it = chunks.find(Coords(x, y, z));
    if (it == chunks.end())
        throw std::out_of_range("Requested chunk is not loaded");

    return it->second;
}

Block* World::getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) {
    return const_cast<Block*>(static_cast<const World&>(*this).getBlock(chunkPosition, blockPosition));
}

const Block* World::getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) const {
    if (!isChunkPositionInBounds(chunkPosition))
        return nullptr;

    auto it = chunks.find(Coords(chunkPosition));
    if (it == chunks.end())
        return nullptr;

    return it->second.getLocalBlock(blockPosition);
}
