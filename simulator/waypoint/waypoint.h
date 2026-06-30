#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>

// Forward declare the Terrain class so we can pass it in
namespace Map { class Terrain; } 

class WaypointSystem {
public:
    WaypointSystem();
    ~WaypointSystem();

    bool initialize();
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // UPDATED: Now requires the Terrain to calculate a safe height
    void spawnNext(const glm::vec3& aircraftPosNed, const Map::Terrain& terrain);
    
    // UPDATED: Now requires the Terrain (so it can pass it to spawnNext on a hit)
    bool checkCollision(const glm::vec3& aircraftPosNed, const Map::Terrain& terrain);

    glm::vec3 getPositionNed() const { return posNed_; }
    float getRadius() const { return currentRadius_; }

    // Adjustable Parameters
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 0.0f); 
    float minRange = 800.0f;
    float maxRange = 1500.0f;
    float minHeight = -200.0f;  
    float maxHeight = -500.0f; 
    float minRadius = 30.0f;
    float maxRadius = 100.0f;

private:
    float currentRadius_;
    glm::vec3 posNed_;
    
    GLuint vao_, vbo_, shaderProgram_;
    int vertexCount_;
    
    GLuint compileShader(GLenum type, const char* source);
};