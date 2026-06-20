#ifndef CHUNK_H
#define CHUNK_H

#include "block.h"
#include "coords.h"

#include <unordered_map>
#include <vector>

class Shader;
class World;

constexpr unsigned CHUNK_WIDTH = 16;
constexpr unsigned CHUNK_HEIGHT = 16;
constexpr unsigned CHUNK_DEPTH = 16;
constexpr unsigned CHUNK_SIZE = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;

constexpr float BLOCK_SIZE = 0.1f;
constexpr float CHUNK_WORLD_WIDTH = CHUNK_WIDTH * BLOCK_SIZE;
constexpr float CHUNK_WORLD_HEIGHT = CHUNK_HEIGHT * BLOCK_SIZE;
constexpr float CHUNK_WORLD_DEPTH = CHUNK_DEPTH * BLOCK_SIZE;

inline glm::ivec3 indexToLocal(int i) {
    int x = i % CHUNK_WIDTH;
    int y = (i / CHUNK_WIDTH) % CHUNK_HEIGHT;
    int z = i / (CHUNK_WIDTH * CHUNK_HEIGHT);

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
    std::vector<float> buildMeshVertices() const;
    void setMeshVertices(std::vector<float>&& vertices);
    void uploadMesh();
    void drawChunk(Shader* shader);
    void releaseGpuResources();

    glm::ivec3 chunkPos = glm::ivec3(0);

private:
    bool isLocalPositionInBounds(const glm::ivec3& localPos) const;
    Block* getLocalBlock(const glm::ivec3& localPos);
    const Block* getLocalBlock(const glm::ivec3& localPos) const;
    const Block* getBlock(const glm::ivec3& localPos) const;

    World* world = nullptr;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    bool gpuUploadDirty = false;
    int vertexCount = 0;
    std::vector<float> meshVertices;
    std::unordered_map<Coords, Block> blocks;

    friend class World;
};

#endif
