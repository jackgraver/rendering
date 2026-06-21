#ifndef WORLD_H
#define WORLD_H

#include <condition_variable>
#include <cstddef>
#include <glm/glm.hpp>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Coords.h"
#include "chunk.h"

class Shader;
class World;

class World {
public:
    static constexpr int LOAD_RADIUS = 8;

    World();
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    void requestChunks();
    void processNewChunks();
    void loadNewChunks();

    Chunk& getChunk(int x, int y, int z);
    const Chunk& getChunk(int x, int y, int z) const;
    Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition);
    const Block* getBlock(const glm::ivec3& chunkPosition, const glm::ivec3& blockPosition) const;

    Coords centerChunk;
    Coords playerChunk;
    std::unordered_map<Coords, Chunk> chunks;

private:
    static constexpr std::size_t kMaxChunkIntegrationsPerFrame = 4;
    static constexpr std::size_t kMaxMeshResultsPerFrame = 4;
    static constexpr std::size_t kMaxGpuUploadsPerFrame = 2;

    enum class WorkerJobType {
        Populate,
        Mesh,
    };

    struct WorkerJob {
        WorkerJobType type;
        Coords coords;
    };

    struct MeshResult {
        Coords coords;
        std::vector<float> vertices;
    };

    void workerLoop();
    void enqueueMeshJob(const Coords& coords);

    std::queue<WorkerJob> pendingJobs;
    std::queue<Chunk> populatedChunkResults;
    std::queue<MeshResult> meshResults;
    std::queue<Coords> pendingGpuUploads;
    std::unordered_set<Coords> requestedChunks;
    std::unordered_set<Coords> queuedMeshJobs;
    std::unordered_set<Coords> queuedGpuUploads;

    std::thread workerThread;
    std::mutex jobsMutex;
    std::condition_variable jobsCondition;
    std::mutex completedMutex;
    std::mutex chunksMutex;
    bool workerRunning = true;
};

#endif
