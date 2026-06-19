#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>
#include "Coords.h"
#include "chunk.h"

// MUST be here (used in Chunk layout)

class Shader;
class World;

class World {
public:
    static constexpr int LOAD_RADIUS = 8;

    World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    void updateLoadedChunks();
    Chunk& getChunk(int x, int y, int z);
    const Chunk& getChunk(int x, int y, int z) const;
    Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition);
    const Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) const;

    Coords centerChunk;
    std::unordered_map<Coords, Chunk> chunks;
};

#endif
