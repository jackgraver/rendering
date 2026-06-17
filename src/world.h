#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>
#include "chunk.h"

// MUST be here (used in Chunk layout)

class Shader;
class World;

class World {
public:
    static constexpr int SIZE = 4;

    World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    Chunk& getChunk(int x, int y, int z);
    const Chunk& getChunk(int x, int y, int z) const;
    Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition);
    const Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) const;

    Chunk chunks[SIZE * SIZE * SIZE];
};

#endif
