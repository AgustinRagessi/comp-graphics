#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace Map {

/**
 * @brief Configuration for terrain generation
 */
struct TerrainConfig {
    // Chunk configuration
    int viewDistance = 4;       // Chunks to render in each direction from camera
    int chunkResolution = 64;   // Vertices per chunk edge
    float chunkSize = 100.0f;   // World units per chunk
    
    // Noise configuration
    float heightScale = 100.0f; // Maximum terrain height
    float noiseScale = 0.02f;   // Frequency of noise (smaller = smoother)
    int octaves = 6;            // Number of noise layers
    float persistence = 0.5f;   // How much each octave contributes
    float lacunarity = 2.0f;    // Frequency multiplier per octave
    
    // Texture configuration
    float textureRepeat = 8.0f; // How many times texture repeats per chunk
    
    // Performance
    int maxChunksPerFrame = 2;  // Max new chunks to generate per frame
};

/**
 * @brief Represents a single terrain chunk
 */
struct TerrainChunk {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int chunkX = 0;             // Grid X coordinate
    int chunkZ = 0;             // Grid Z coordinate
    glm::vec2 worldPos;         // World position of chunk origin
    int indexCount = 0;
};

// Hash function for chunk coordinates
struct ChunkKeyHash {
    std::size_t operator()(const std::pair<int, int>& key) const {
        return std::hash<int>()(key.first) ^ (std::hash<int>()(key.second) << 16);
    }
};

using ChunkKey = std::pair<int, int>;  // (chunkX, chunkZ)
using ChunkMap = std::unordered_map<ChunkKey, TerrainChunk, ChunkKeyHash>;

/**
 * @brief Procedural terrain generator with dynamic chunk loading
 * 
 * Uses Fractal Brownian Motion (fBm) for natural-looking terrain.
 * Dynamically generates and unloads chunks based on camera position.
 * Provides collision detection via height sampling (works for any position).
 */
class Terrain {
public:
    Terrain();
    ~Terrain();

    // No copying
    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    /**
     * @brief Initialize terrain system
     * @param config Terrain generation parameters
     * @param texturePath Path to grass texture
     * @return true if initialization succeeded
     */
    bool initialize(const TerrainConfig& config, const std::string& texturePath);

    /**
     * @brief Update visible chunks based on camera position
     * Call this every frame before render()
     * @param cameraPos Current camera world position
     */
    void update(const glm::vec3& cameraPos);
    void renderDepth(GLuint depthShader, const glm::mat4& lightSpaceMatrix);
    /**
     * @brief Render all loaded terrain chunks
     * @param mvp Combined model-view-projection matrix
     * @param cameraPos Camera position in world space
     * @param lightDir Direction of sunlight (normalized)
     */
    void render(const glm::mat4& mvp,
            const glm::mat4& model,
            const glm::mat4& lightSpaceMatrix,
            GLuint shadowMap,
            const glm::vec3& cameraPos,
            const glm::vec3& lightDir = glm::vec3(0.5f, -1.0f, 0.3f));

    /**
     * @brief Get terrain height at any world position (for collision)
     * Works regardless of whether chunk is loaded - uses procedural noise
     */
    float getHeightAt(float x, float z) const;

    /**
     * @brief Get terrain normal at world position (for collision response)
     */
    glm::vec3 getNormalAt(float x, float z) const;

    /**
     * @brief Check if a point is below terrain (collision)
     */
    bool checkCollision(const glm::vec3& position) const;

    /**
     * @brief Get number of currently loaded chunks
     */
    size_t getLoadedChunkCount() const { return chunks_.size(); }

    void cleanup();

private:
    TerrainConfig config_;
    ChunkMap chunks_;           // Loaded chunks indexed by grid position
    GLuint texture_;
    GLuint shaderProgram_;
    
    // Current camera chunk (for tracking movement)
    int lastCamChunkX_ = INT_MAX;
    int lastCamChunkZ_ = INT_MAX;

    // Noise generation (procedural - works for any coordinate)
    float interpolatedNoise2D(float x, float z) const;
    float fbm(float x, float z) const;
    int hash(int x, int z) const;
    
    // Chunk management
    bool generateChunk(int chunkX, int chunkZ);
    void unloadChunk(const ChunkKey& key);
    ChunkKey worldToChunk(float worldX, float worldZ) const;
    
    // Shader handling
    bool createShaderProgram();
    GLuint compileShader(GLenum type, const char* source);
    bool loadTexture(const std::string& path);
};

} // namespace Map
