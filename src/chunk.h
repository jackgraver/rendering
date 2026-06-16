#ifndef CHUNK_H
#define CHUNK_H

#include "block.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

class Shader;
class World;

struct IVec3Hash {
    std::size_t operator()(const glm::ivec3& v) const {
        std::size_t x = std::hash<int>{}(v.x);
        std::size_t y = std::hash<int>{}(v.y);
        std::size_t z = std::hash<int>{}(v.z);

        return x ^ (y << 1) ^ (z << 2);
    }
};

constexpr unsigned CHUNK_COLS = 2;
constexpr unsigned CHUNK_ROWS = 8;
constexpr unsigned CHUNK_HEIGHT = 8;
constexpr unsigned CHUNK_SIZE = CHUNK_ROWS * CHUNK_COLS * CHUNK_HEIGHT;

inline glm::ivec3 indexToLocal(int i) {
    int x = i % CHUNK_ROWS;
    int y = (i / CHUNK_ROWS) % CHUNK_COLS;
    int z = i / (CHUNK_ROWS * CHUNK_COLS);

    return glm::ivec3(x, y, z);
}

inline int floorDiv(int value, int divisor) {
    int quotient = value / divisor;
    int remainder = value % divisor;

    if (remainder < 0)
        quotient -= 1;

    return quotient;
}

inline int positiveMod(int value, int divisor) {
    int result = value % divisor;

    if (result < 0)
        result += divisor;

    return result;
}

struct FaceData {
    glm::ivec3 neighborOffset;
    glm::vec3 normal;
    glm::vec3 vertices[6];
};

extern const FaceData faces[6];

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
    Chunk(World* world, int x, int y, int z);

    void setWorld(World* world);
    void populateChunk();
    void buildChunkMesh();
    void drawChunk(Shader* shader);
    void releaseGpuResources();

private:
    bool isLocalPositionInBounds(const glm::ivec3& localPos) const;
    Block* getLocalBlock(const glm::ivec3& localPos);
    const Block* getLocalBlock(const glm::ivec3& localPos) const;
    const Block* getBlock(const glm::ivec3& localPos) const;

    World* world = nullptr;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    bool meshDirty = true;
    int vertexCount = 0;
    glm::ivec3 chunkPos = glm::ivec3(0);
    // Map of local block positions to blocks in the chunk
    std::unordered_map<glm::ivec3, Block, IVec3Hash> blocks;

    friend class World;
};

#endif
