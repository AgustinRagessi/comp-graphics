#include "waypoint.h"
#include "../map/terrain.h" // Include terrain to query heights
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cstdlib>
#include "../helper/coordinateTranslate.h"

static const char* wpVS = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* wpFS = R"(
#version 460 core
uniform vec3 color;
out vec4 FragColor;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

WaypointSystem::WaypointSystem() : currentRadius_(50.0f), vao_(0), vbo_(0), shaderProgram_(0), vertexCount_(0) {}

WaypointSystem::~WaypointSystem() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
}

bool WaypointSystem::initialize() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, wpVS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, wpFS);
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vs);
    glAttachShader(shaderProgram_, fs);
    glLinkProgram(shaderProgram_);
    
    glDeleteShader(vs);
    glDeleteShader(fs);

    std::vector<glm::vec3> vertices;
    int segments = 36;
    for(int i = 0; i < segments; ++i) {
        float a1 = i * (2.0f * 3.14159f / segments);
        float a2 = (i+1) * (2.0f * 3.14159f / segments);
        
        vertices.push_back(glm::vec3(cos(a1), sin(a1), 0.0f));
        vertices.push_back(glm::vec3(cos(a2), sin(a2), 0.0f));
        vertices.push_back(glm::vec3(cos(a1), 0.0f, sin(a1)));
        vertices.push_back(glm::vec3(cos(a2), 0.0f, sin(a2)));
        vertices.push_back(glm::vec3(0.0f, cos(a1), sin(a1)));
        vertices.push_back(glm::vec3(0.0f, cos(a2), sin(a2)));
    }
    
    vertexCount_ = vertices.size();

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return true;
}

// ============================================================================
// TERRAIN-AWARE SPAWN LOGIC
// ============================================================================
void WaypointSystem::spawnNext(const glm::vec3& aircraftPosNed, const Map::Terrain& terrain) {
    // Generate a random polar coordinate (angle and distance) relative to the aircraft
    float angle = (rand() % 360) * (3.14159f / 180.0f);
    float distance = minRange + (rand() % (int)(maxRange - minRange));

    // Convert polar offset to Cartesian NED (North, East, Down) 
    posNed_.x = aircraftPosNed.x + cos(angle) * distance;
    posNed_.y = aircraftPosNed.y + sin(angle) * distance;

    // Coordinate Bridge: The terrain heightmap function expects World coordinates (X = East, Z = South).
    // We package the X/Y into a dummy vector to use our coordinate translator securely.
    glm::vec3 dummyNed(posNed_.x, posNed_.y, 0.0f);
    glm::vec3 worldPos = Helper::nedToWorldPosition(dummyNed);
    float groundHeightWorld = terrain.getHeightAt(worldPos.x, worldPos.z);

    // Revert the World height back to the physics engine's NED scale (where Down is positive Z)
    float groundHeightNed = -groundHeightWorld;

    // Ceiling Floor Logic: Calculate safe altitude boundaries
    float safeClearanceNed = 0.0f;
    float lowestAllowedNed = groundHeightNed + safeClearanceNed;

    // Ensure the predefined ceiling (maxHeight) isn't accidentally placing the waypoint underground 
    // if the spawn point is on top of a tall mountain.
    float highestAllowedNed = maxHeight; 
    if (highestAllowedNed > lowestAllowedNed - 200.0f) {
        highestAllowedNed = lowestAllowedNed - 200.0f; 
    }

    // Assign final altitude within the safe vertical window
    float heightRange = lowestAllowedNed - highestAllowedNed;
    posNed_.z = lowestAllowedNed - (rand() % (int)heightRange);

    currentRadius_ = minRadius + (rand() % (int)(maxRadius - minRadius));

    std::cout << "[WAYPOINT] Target spawned at Alt: " << -posNed_.z 
              << "m (Terrain below is " << groundHeightWorld << "m)" << std::endl;
}

bool WaypointSystem::checkCollision(const glm::vec3& aircraftPosNed, const Map::Terrain& terrain) {
    float dist = glm::distance(aircraftPosNed, posNed_);
    if (dist < currentRadius_) {
        spawnNext(aircraftPosNed, terrain); // Pass the terrain along so it can spawn safely!
        return true;
    }
    return false;
}

void WaypointSystem::render(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram_);
    
    glm::vec3 worldPos = Helper::nedToWorldPosition(posNed_);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, worldPos);
    model = glm::scale(model, glm::vec3(currentRadius_));
    
    glm::mat4 mvp = projection * view * model;
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(glGetUniformLocation(shaderProgram_, "color"), 1, glm::value_ptr(color));
    
    glLineWidth(3.0f);
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, vertexCount_);
    glBindVertexArray(0);
}

GLuint WaypointSystem::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}