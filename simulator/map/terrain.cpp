#include "terrain.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Map {

//=============================================================================
// TERRAIN SHADERS
//=============================================================================

static const char* terrainVertexShader = R"(
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

uniform mat4 mvp;                  
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    TexCoord = aTexCoord; 
    
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // Use the combined MVP matrix to place the terrain on screen
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* terrainFragmentShader = R"(
#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D terrainTexture;
uniform sampler2D shadowMap; 

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 fogColor;
uniform vec3 cameraPos;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec4 texColor = texture(terrainTexture, TexCoord);
    
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    
    // Calculate diffuse sunlight
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Check if pixel is hidden from the sun
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDirection);
    
    // Apply lighting
    vec3 lighting = ambientColor + (1.0 - shadow) * diffuse;
    vec3 result = lighting * texColor.rgb;
    
    // Distance fog
    float distance = length(FragPos - cameraPos);
    float fogFactor = clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);
    
    FragColor = vec4(result, 1.0);
}
)";

//=============================================================================
// NOISE IMPLEMENTATION
//=============================================================================

static const int PERM[] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

Terrain::Terrain()
    : texture_(0), shaderProgram_(0) {}

Terrain::~Terrain() {
    cleanup();
}

int Terrain::hash(int x, int z) const {
    x = x & 255;
    z = z & 255;
    return PERM[(x + PERM[z]) & 255];
}

static float smoothstep(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float Terrain::interpolatedNoise2D(float x, float z) const {
    int xi = static_cast<int>(std::floor(x));
    int zi = static_cast<int>(std::floor(z));
    
    float xf = x - xi;
    float zf = z - zi;
    
    float n00 = static_cast<float>(hash(xi, zi)) / 255.0f;
    float n10 = static_cast<float>(hash(xi + 1, zi)) / 255.0f;
    float n01 = static_cast<float>(hash(xi, zi + 1)) / 255.0f;
    float n11 = static_cast<float>(hash(xi + 1, zi + 1)) / 255.0f;
    
    float u = smoothstep(xf);
    float v = smoothstep(zf);
    
    float nx0 = n00 * (1.0f - u) + n10 * u;
    float nx1 = n01 * (1.0f - u) + n11 * u;
    
    return nx0 * (1.0f - v) + nx1 * v;
}

// Fractional Brownian Motion (fBm) combines multiple layers of noise to create realistic terrain features. 
 //Each octave contributes a different frequency and amplitude, allowing for both large-scale and fine details in the terrain.
float Terrain::fbm(float x, float z) const {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < config_.octaves; i++) {
        total += interpolatedNoise2D(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= config_.persistence;
        frequency *= config_.lacunarity;
    }
    
    return total / maxValue;
}

//=============================================================================
// INITIALIZATION
//=============================================================================

bool Terrain::initialize(const TerrainConfig& config, const std::string& texturePath) {
    config_ = config;
    
    std::cout << "\n=== Initializing Dynamic Terrain ===" << std::endl;
    std::cout << "View distance: " << config.viewDistance << " chunks" << std::endl;
    std::cout << "Resolution: " << config.chunkResolution << " vertices per edge" << std::endl;
    std::cout << "Chunk size: " << config.chunkSize << " world units" << std::endl;
    
    if (!createShaderProgram()) {
        std::cerr << "Failed to create terrain shader" << std::endl;
        return false;
    }
    
    if (!loadTexture(texturePath)) {
        std::cerr << "Failed to load terrain texture: " << texturePath << std::endl;
        return false;
    }
    
    // Pre-generate all initial chunks around origin (0,0)
    // This ensures terrain is complete before first frame renders
    int viewDist = config.viewDistance;
    int totalChunks = (2 * viewDist + 1) * (2 * viewDist + 1);
    int generated = 0;
    
    std::cout << "Pre-generating " << totalChunks << " initial chunks..." << std::flush;
    
    for (int z = -viewDist; z <= viewDist; z++) {
        for (int x = -viewDist; x <= viewDist; x++) {
            generateChunk(x, z);
            generated++;
            
            // Progress indicator every 10 chunks
            if (generated % 10 == 0) {
                std::cout << "." << std::flush;
            }
        }
    }
    
    // Initialize camera tracking to origin chunk
    lastCamChunkX_ = 0;
    lastCamChunkZ_ = 0;
    
    std::cout << " Done!" << std::endl;
    std::cout << "=== Terrain ready (" << chunks_.size() << " chunks loaded) ===\n" << std::endl;
    return true;
}

//=============================================================================
// DYNAMIC CHUNK MANAGEMENT
//=============================================================================

ChunkKey Terrain::worldToChunk(float worldX, float worldZ) const {
    int cx = static_cast<int>(std::floor(worldX / config_.chunkSize));
    int cz = static_cast<int>(std::floor(worldZ / config_.chunkSize));
    return {cx, cz};
}

void Terrain::update(const glm::vec3& cameraPos) {
    // Map current camera coordinates to a grid-based chunk key
    auto [camChunkX, camChunkZ] = worldToChunk(cameraPos.x, cameraPos.z);
    
    // Optimization: Halt all logic if the camera hasn't crossed a chunk boundary
    if (camChunkX == lastCamChunkX_ && camChunkZ == lastCamChunkZ_) {
        return;
    }
    lastCamChunkX_ = camChunkX;
    lastCamChunkZ_ = camChunkZ;
    
    int viewDist = config_.viewDistance;
    
    // 1. CHUNK DISCOVERY: Scan the view distance radius and identify missing chunks
    std::vector<ChunkKey> chunksToLoad;
    for (int z = camChunkZ - viewDist; z <= camChunkZ + viewDist; z++) {
        for (int x = camChunkX - viewDist; x <= camChunkX + viewDist; x++) {
            ChunkKey key = {x, z};
            if (chunks_.find(key) == chunks_.end()) {
                chunksToLoad.push_back(key);
            }
        }
    }
    
    // 2. CHUNK PRIORITIZATION: Sort chunks radially so those closest to the player spawn first
    std::sort(chunksToLoad.begin(), chunksToLoad.end(), 
        [camChunkX, camChunkZ](const ChunkKey& a, const ChunkKey& b) {
            int distA = (a.first - camChunkX) * (a.first - camChunkX) + (a.second - camChunkZ) * (a.second - camChunkZ);
            int distB = (b.first - camChunkX) * (b.first - camChunkX) + (b.second - camChunkZ) * (b.second - camChunkZ);
            return distA < distB;
        });
    
    // 3. CHUNK GENERATION: Throttle creation per frame to prevent stuttering/frame drops
    int maxPerFrame = std::max(config_.maxChunksPerFrame, 8);
    int chunksGenerated = 0;
    for (const auto& key : chunksToLoad) {
        if (chunksGenerated >= maxPerFrame) break;
        if (generateChunk(key.first, key.second)) {
            chunksGenerated++;
        }
    }
    
    // 4. CHUNK EVICTION: Identify chunks outside the view distance + hysteresis margin and delete their OpenGL buffers
    std::vector<ChunkKey> chunksToUnload;
    int unloadDist = viewDist + 2; // Hysteresis buffer prevents rapid load/unload cycling if hovering on a boundary
    
    for (const auto& [key, chunk] : chunks_) {
        int dx = key.first - camChunkX;
        int dz = key.second - camChunkZ;
        if (std::abs(dx) > unloadDist || std::abs(dz) > unloadDist) {
            chunksToUnload.push_back(key);
        }
    }
    
    for (const auto& key : chunksToUnload) {
        unloadChunk(key);
    }
}

bool Terrain::generateChunk(int chunkX, int chunkZ) {
    TerrainChunk chunk;
    chunk.chunkX = chunkX;
    chunk.chunkZ = chunkZ;
    
    int resolution = config_.chunkResolution;
    float chunkSize = config_.chunkSize;
    
    // Calculate world position of chunk origin
    float worldStartX = chunkX * chunkSize;
    float worldStartZ = chunkZ * chunkSize;
    chunk.worldPos = glm::vec2(worldStartX, worldStartZ);
    
    float step = chunkSize / (resolution - 1);
    
    // First pass: generate height map (faster than re-computing noise for normals)
    std::vector<float> heightMap(resolution * resolution);
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            float worldX = worldStartX + x * step;
            float worldZ = worldStartZ + z * step;
            heightMap[z * resolution + x] = getHeightAt(worldX, worldZ);
        }
    }
    
    // Helper to get height from map with bounds checking
    auto getHeight = [&](int x, int z) -> float {
        x = std::max(0, std::min(resolution - 1, x));
        z = std::max(0, std::min(resolution - 1, z));
        return heightMap[z * resolution + x];
    };
    
    // Second pass: generate vertices with normals computed from height map
    std::vector<float> vertices;
    vertices.reserve(resolution * resolution * 8);
    
    for (int z = 0; z < resolution; z++) {
        for (int x = 0; x < resolution; x++) {
            float worldX = worldStartX + x * step;
            float worldZ = worldStartZ + z * step;
            float height = heightMap[z * resolution + x];
            
            // Position
            vertices.push_back(worldX);
            vertices.push_back(height);
            vertices.push_back(worldZ);
            
            // Normal computed from neighboring heights (finite differences)
            float hL = getHeight(x - 1, z);
            float hR = getHeight(x + 1, z);
            float hD = getHeight(x, z - 1);
            float hU = getHeight(x, z + 1);
            
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * step, hD - hU));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            
            // Texture coordinates
            float u = (float)x / (resolution - 1) * config_.textureRepeat;
            float v = (float)z / (resolution - 1) * config_.textureRepeat;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }
    
    // Generate indices (same topology for all chunks)
    std::vector<unsigned int> indices;
    indices.reserve((resolution - 1) * (resolution - 1) * 6);
    
    for (int z = 0; z < resolution - 1; z++) {
        for (int x = 0; x < resolution - 1; x++) {
            int topLeft = z * resolution + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * resolution + x;
            int bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    chunk.indexCount = indices.size();
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &chunk.vao);
    glGenBuffers(1, &chunk.vbo);
    glGenBuffers(1, &chunk.ebo);
    
    glBindVertexArray(chunk.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                 vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 
                         (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                         (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    // Add to chunk map
    ChunkKey key = {chunkX, chunkZ};
    chunks_[key] = chunk;
    
    return true;
}

void Terrain::unloadChunk(const ChunkKey& key) {
    auto it = chunks_.find(key);
    if (it != chunks_.end()) {
        TerrainChunk& chunk = it->second;
        if (chunk.vao) glDeleteVertexArrays(1, &chunk.vao);
        if (chunk.vbo) glDeleteBuffers(1, &chunk.vbo);
        if (chunk.ebo) glDeleteBuffers(1, &chunk.ebo);
        chunks_.erase(it);
    }
}

//=============================================================================
// COLLISION / HEIGHT QUERIES
//=============================================================================

// GLSL-style smoothstep utility for blending biomes naturally
static float smoothstep_glsl(float edge0, float edge1, float x) {
    float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

float Terrain::getHeightAt(float x, float z) const {
    // 1. BIOME MASK: Sweeping, low-frequency noise to map geographic regions
    float biomeFreq = config_.noiseScale * 0.08f; 
    float biomeNoise = fbm(x * biomeFreq, z * biomeFreq);
    
    // Create a stark transition: 0.0 = Plains, 1.0 = Mountains
    // Anything between 0.40 and 0.60 becomes foothills bridging the two.
    float mountainMask = smoothstep_glsl(0.40f, 0.60f, biomeNoise);

    // 2. PLAINS: Smooth, rolling hills (low amplitude)
    float plainFreq = config_.noiseScale * 1.5f;
    float plainNoise = fbm(x * plainFreq, z * plainFreq);
    float plainHeight = plainNoise * (config_.heightScale * 0.2f); // Max ~20 meters

    // 3. MOUNTAINS: Jagged, imposing peaks (high amplitude)
    float mountFreq = config_.noiseScale * 0.8f;
    float mountNoise = fbm(x * mountFreq, z * mountFreq);
    
    // std::pow curves the terrain. It forces the lower noise values to stay low (wide mountain valleys) 
    // while the high values shoot upward (sharp, distinct peaks).
    float mountHeight = std::pow(mountNoise, 2.5f) * (config_.heightScale * 3.5f); // Max ~350 meters

    // 4. BLEND: Mix the two terrains together using the biome mask
    float finalHeight = (plainHeight * (1.0f - mountainMask)) + (mountHeight * mountainMask);

    // Lower the entire map slightly so the lowest plains settle nicely
    return finalHeight - (config_.heightScale * 0.15f);
}

glm::vec3 Terrain::getNormalAt(float x, float z) const {
    float delta = 0.5f;
    float heightL = getHeightAt(x - delta, z);
    float heightR = getHeightAt(x + delta, z);
    float heightD = getHeightAt(x, z - delta);
    float heightU = getHeightAt(x, z + delta);
    
    glm::vec3 normal(
        heightL - heightR,
        2.0f * delta,
        heightD - heightU
    );
    
    return glm::normalize(normal);
}

bool Terrain::checkCollision(const glm::vec3& position) const {
    float terrainHeight = getHeightAt(position.x, position.z);
    return position.y < terrainHeight;
}

//=============================================================================
// RENDERING
//=============================================================================

void Terrain::renderDepth(GLuint depthShader, const glm::mat4& lightSpaceMatrix) {
    if (chunks_.empty()) return;
    
    glUseProgram(depthShader);
    glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    for (const auto& [key, chunk] : chunks_) {
        glBindVertexArray(chunk.vao);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ebo);
        glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void Terrain::render(const glm::mat4& mvp,
                     const glm::mat4& model,
                     const glm::mat4& lightSpaceMatrix,
                     GLuint shadowMap,
                     const glm::vec3& cameraPos,
                     const glm::vec3& lightDir) {
    if (chunks_.empty()) return;
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    
    glUseProgram(shaderProgram_);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glUniform3fv(glGetUniformLocation(shaderProgram_, "lightDir"), 1, glm::value_ptr(glm::normalize(lightDir)));
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightColor"), 1.0f, 0.95f, 0.8f);
    glUniform3f(glGetUniformLocation(shaderProgram_, "ambientColor"), 0.3f, 0.35f, 0.4f);
    
    glUniform1f(glGetUniformLocation(shaderProgram_, "fogStart"), 200.0f);
    glUniform1f(glGetUniformLocation(shaderProgram_, "fogEnd"), 500.0f);
    glUniform3f(glGetUniformLocation(shaderProgram_, "fogColor"), 0.7f, 0.8f, 0.9f);
    glUniform3fv(glGetUniformLocation(shaderProgram_, "cameraPos"), 1, glm::value_ptr(cameraPos));
    
    // Bind Textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "terrainTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glUniform1i(glGetUniformLocation(shaderProgram_, "shadowMap"), 1);
    
    for (const auto& [key, chunk] : chunks_) {
        glBindVertexArray(chunk.vao);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ebo);
        glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
    // Reset active texture to 0 to prevent conflicts with Skybox/HUD
    glActiveTexture(GL_TEXTURE0); 
}

//=============================================================================
// RESOURCE MANAGEMENT
//=============================================================================

bool Terrain::createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, terrainVertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, terrainFragmentShader);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    int success;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "Terrain shader linking failed:\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

GLuint Terrain::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Terrain shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

bool Terrain::loadTexture(const std::string& path) {
    std::cout << "Loading terrain texture: " << path << std::endl;
    
    stbi_set_flip_vertically_on_load(true);
    
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        const char* reason = stbi_failure_reason();
        std::cerr << "Failed to load texture: " << (reason ? reason : "Unknown") << std::endl;
        return false;
    }
    
    std::cout << "  Loaded: " << width << "x" << height << " with " << channels << " channels" << std::endl;
    
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    float maxAniso;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    
    stbi_image_free(data);
    
    return true;
}

void Terrain::cleanup() {
    // Unload all chunks
    for (auto& [key, chunk] : chunks_) {
        if (chunk.vao) glDeleteVertexArrays(1, &chunk.vao);
        if (chunk.vbo) glDeleteBuffers(1, &chunk.vbo);
        if (chunk.ebo) glDeleteBuffers(1, &chunk.ebo);
    }
    chunks_.clear();
    
    if (texture_) glDeleteTextures(1, &texture_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    
    texture_ = 0;
    shaderProgram_ = 0;
}

} // namespace Map
