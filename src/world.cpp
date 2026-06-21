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
    playerChunk = centerChunk;

    const int worldDiameter = (LOAD_RADIUS * 2) + 1;
    chunks.reserve(worldDiameter * worldDiameter * kWorldChunkHeight);

    workerThread = std::thread(&World::workerLoop, this);
}

World::~World() {
    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        workerRunning = false;
    }
    jobsCondition.notify_all();

    if (workerThread.joinable())
        workerThread.join();
}

void World::requestChunks() {
    centerChunk = playerChunk;

    const int minX = centerChunk.x() - LOAD_RADIUS;
    const int maxX = centerChunk.x() + LOAD_RADIUS;
    const int minY = 0;
    const int maxY = kWorldChunkHeight - 1;
    const int minZ = centerChunk.z() - LOAD_RADIUS;
    const int maxZ = centerChunk.z() + LOAD_RADIUS;

    std::vector<WorkerJob> jobsToQueue;

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                const Coords chunkCoords(x, y, z);
                if (chunks.find(chunkCoords) != chunks.end())
                    continue;

                if (!requestedChunks.insert(chunkCoords).second)
                    continue;

                jobsToQueue.push_back({WorkerJobType::Populate, chunkCoords});
            }
        }
    }

    if (jobsToQueue.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        for (WorkerJob& job : jobsToQueue)
            pendingJobs.push(std::move(job));
    }

    jobsCondition.notify_one();
}

void World::processNewChunks() {
    std::size_t processedChunks = 0;

    while (processedChunks < kMaxChunkIntegrationsPerFrame) {
        Chunk chunk;
        {
            std::lock_guard<std::mutex> lock(completedMutex);
            if (populatedChunkResults.empty())
                break;

            chunk = std::move(populatedChunkResults.front());
            populatedChunkResults.pop();
        }

        const Coords chunkCoords(chunk.chunkPos);
        chunk.setWorld(this);

        {
            std::lock_guard<std::mutex> lock(chunksMutex);
            chunks.insert_or_assign(chunkCoords, std::move(chunk));
        }

        enqueueMeshJob(chunkCoords);
        for (const FaceData& face : faces)
            enqueueMeshJob(chunkCoords + face.neighborOffset);

        ++processedChunks;
    }
}

void World::loadNewChunks() {
    std::size_t appliedMeshResults = 0;

    while (appliedMeshResults < kMaxMeshResultsPerFrame) {
        MeshResult meshResult;
        {
            std::lock_guard<std::mutex> lock(completedMutex);
            if (meshResults.empty())
                break;

            meshResult = std::move(meshResults.front());
            meshResults.pop();
        }

        bool chunkExists = false;
        {
            std::lock_guard<std::mutex> lock(chunksMutex);
            auto it = chunks.find(meshResult.coords);
            if (it != chunks.end()) {
                it->second.setMeshVertices(std::move(meshResult.vertices));
                chunkExists = true;
            }
        }

        if (chunkExists && queuedGpuUploads.insert(meshResult.coords).second)
            pendingGpuUploads.push(meshResult.coords);

        queuedMeshJobs.erase(meshResult.coords);
        ++appliedMeshResults;
    }

    std::size_t uploadedMeshes = 0;
    while (uploadedMeshes < kMaxGpuUploadsPerFrame) {
        if (pendingGpuUploads.empty())
            break;

        const Coords coords = pendingGpuUploads.front();
        pendingGpuUploads.pop();

        Chunk* chunkToUpload = nullptr;
        {
            std::lock_guard<std::mutex> lock(chunksMutex);
            auto it = chunks.find(coords);
            if (it != chunks.end())
                chunkToUpload = &it->second;
        }

        if (chunkToUpload != nullptr) {
            chunkToUpload->uploadMesh();
            ++uploadedMeshes;
        }

        queuedGpuUploads.erase(coords);
    }
}

void World::workerLoop() {
    while (true) {
        WorkerJob job;
        {
            std::unique_lock<std::mutex> lock(jobsMutex);
            jobsCondition.wait(lock, [this] { return !workerRunning || !pendingJobs.empty(); });

            if (!workerRunning && pendingJobs.empty())
                return;

            job = std::move(pendingJobs.front());
            pendingJobs.pop();
        }

        if (job.type == WorkerJobType::Populate) {
            Chunk chunk(this, job.coords.x(), job.coords.y(), job.coords.z());
            chunk.populateChunk();

            std::lock_guard<std::mutex> lock(completedMutex);
            populatedChunkResults.push(std::move(chunk));
            continue;
        }

        MeshResult meshResult;
        meshResult.coords = job.coords;
        {
            std::lock_guard<std::mutex> lock(chunksMutex);
            auto it = chunks.find(job.coords);
            if (it != chunks.end()) {
                meshResult.vertices = it->second.buildMeshVertices();
            }
        }

        std::lock_guard<std::mutex> lock(completedMutex);
        meshResults.push(std::move(meshResult));
    }
}

void World::enqueueMeshJob(const Coords& coords) {
    if (queuedMeshJobs.find(coords) != queuedMeshJobs.end())
        return;

    if (chunks.find(coords) == chunks.end())
        return;

    queuedMeshJobs.insert(coords);

    {
        std::lock_guard<std::mutex> lock(jobsMutex);
        pendingJobs.push({WorkerJobType::Mesh, coords});
    }

    jobsCondition.notify_one();
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
