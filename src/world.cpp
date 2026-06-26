#include "world.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <chrono>
#include <iostream>


namespace {
constexpr int kWorldChunkHeight = 4;
constexpr double kSlowStreamingStepMs = 6.0;
constexpr double kSlowGpuUploadMs = 3.0;
constexpr double kSlowWorkerJobMs = 8.0;

using Clock = std::chrono::steady_clock;

std::mutex gDebugLogMutex;

bool isChunkPositionInBounds(const glm::ivec3& chunkPosition) {
    return chunkPosition.y >= 0 && chunkPosition.y < kWorldChunkHeight;
}

Coords worldPositionToChunkCoords(const glm::vec3& worldPosition) {
    return Coords(
        static_cast<int>(std::floor(worldPosition.x / CHUNK_WORLD_WIDTH)),
        static_cast<int>(std::floor(worldPosition.y / CHUNK_WORLD_HEIGHT)),
        static_cast<int>(std::floor(worldPosition.z / CHUNK_WORLD_DEPTH))
    );
}

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string formatCoords(const Coords& coords) {
    std::ostringstream stream;
    stream << '(' << coords.x() << ", " << coords.y() << ", " << coords.z() << ')';
    return stream.str();
}

void logStreamingDebug(const std::string& message) {
    std::lock_guard<std::mutex> lock(gDebugLogMutex);
    std::cout << message << std::endl;
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

World::World(): worldShader("src/shaders/lighting_vert.glsl", "src/shaders/lighting_frag.glsl") {
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

void World::requestChunks(const glm::vec3& playerPosition) {
    const auto requestStart = Clock::now();

    playerChunk = worldPositionToChunkCoords(playerPosition);

    const auto processStart = Clock::now();
    const std::size_t processedChunks = processNewChunks();
    const auto processEnd = Clock::now();

    const auto loadStart = Clock::now();
    const LoadNewChunksStats loadStats = loadNewChunks();
    const auto loadEnd = Clock::now();

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

    const std::size_t queuedJobs = jobsToQueue.size();

    if (!jobsToQueue.empty()) {
        std::lock_guard<std::mutex> lock(jobsMutex);
        for (WorkerJob& job : jobsToQueue)
            pendingJobs.push(std::move(job));

        jobsCondition.notify_one();
    }

    const auto requestEnd = Clock::now();
    const double processMs = elapsedMs(processStart, processEnd);
    const double loadMs = elapsedMs(loadStart, loadEnd);
    const double totalMs = elapsedMs(requestStart, requestEnd);

    if (totalMs >= kSlowStreamingStepMs || loadStats.gpuUploadMs >= kSlowGpuUploadMs) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2)
               << "[stream] chunk=" << formatCoords(playerChunk)
               << " processed=" << processedChunks
               << " meshResults=" << loadStats.appliedMeshResults
               << " uploads=" << loadStats.uploadedMeshes
               << " jobsQueued=" << queuedJobs
               << " request=" << totalMs << "ms"
               << " process=" << processMs << "ms"
               << " load=" << loadMs << "ms"
               << " upload=" << loadStats.gpuUploadMs << "ms";
        logStreamingDebug(stream.str());
    }
}

std::size_t World::processNewChunks() {
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

    return processedChunks;
}

World::LoadNewChunksStats World::loadNewChunks() {
    LoadNewChunksStats stats;
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

    stats.appliedMeshResults = appliedMeshResults;

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
            const auto uploadStart = Clock::now();
            chunkToUpload->uploadMesh();
            stats.gpuUploadMs += elapsedMs(uploadStart, Clock::now());
            ++uploadedMeshes;
        }

        queuedGpuUploads.erase(coords);
    }

    stats.uploadedMeshes = uploadedMeshes;
    return stats;
}

void World::drawChunks() {
    glm::mat4 model = glm::mat4(1.0f);
    worldShader.setMat4("model", model);

    for (auto& chunk : chunks) {
        chunk.second.drawChunk(&worldShader);
    }
}

void World::workerLoop() {
    // Background infinite check loop
    while (true) {
        // Current job
        WorkerJob job;
        // Scope block on lock, find current job if any exist or non already executing
        {
            std::unique_lock<std::mutex> lock(jobsMutex);
            jobsCondition.wait(lock, [this] { return !workerRunning || !pendingJobs.empty(); });

            if (!workerRunning && pendingJobs.empty())
                return;

            job = std::move(pendingJobs.front());
            pendingJobs.pop();
        }

        const auto jobStart = Clock::now();

        // Job is Populate, for a given chunk in job we need to populate its blocks then add to queue for mesh building
        if (job.type == WorkerJobType::Populate) {
            Chunk chunk(this, job.coords.x(), job.coords.y(), job.coords.z());
            chunk.populateChunk();

            std::lock_guard<std::mutex> lock(completedMutex);
            populatedChunkResults.push(std::move(chunk));

            const double jobMs = elapsedMs(jobStart, Clock::now());
            if (jobMs >= kSlowWorkerJobMs) {
                std::ostringstream stream;
                stream << std::fixed << std::setprecision(2)
                       << "[worker] Populate " << formatCoords(job.coords)
                       << " took " << jobMs << "ms";
                logStreamingDebug(stream.str());
            }
            continue;
        }
        // Job is Mesh, for a given chunk in job we need to build its mesh vertices then add to queue for GPU upload
        if (job.type == WorkerJobType::Mesh) {
            MeshResult meshResult;
            meshResult.coords = job.coords;

            Chunk chunkCopy;
            {
                std::lock_guard<std::mutex> lock(chunksMutex);
                auto it = chunks.find(job.coords);

                if (it != chunks.end()) {
                    chunkCopy = it->second;
                } else {
                    return; // or handle missing chunk
                }
            }

            meshResult.vertices = chunkCopy.buildMeshVertices();

            std::lock_guard<std::mutex> lock(completedMutex);
            meshResults.push(std::move(meshResult));
        }

        const double jobMs = elapsedMs(jobStart, Clock::now());
        if (jobMs >= kSlowWorkerJobMs) {
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(2)
                   << "[worker] Mesh " << formatCoords(job.coords)
                   << " took " << jobMs << "ms";
            logStreamingDebug(stream.str());
        }
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
