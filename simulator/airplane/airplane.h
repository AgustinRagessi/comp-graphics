#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <dlfdm/fdmsolver.h>

namespace Airplane {

class AirplaneModel {
public:
    AirplaneModel();
    ~AirplaneModel();

    // Prevent copying to avoid OpenGL resource management issues (double freeing buffers)
    AirplaneModel(const AirplaneModel&) = delete;
    AirplaneModel& operator=(const AirplaneModel&) = delete;

    // Loads the OBJ, sets up textures, and compiles the shaders
    bool initialize(const std::string& objPath);
    
    // Main render pass using lighting, camera matrices, and shadow maps
    void render(const dlfdm::FDMSolver& fdm,
            const glm::mat4& view,
            const glm::mat4& projection,
            const glm::mat4& lightSpaceMatrix,
            GLuint shadowMap,
            const glm::vec3& lightDir = glm::vec3(0.5f, -1.0f, 0.3f),
            const glm::vec3& cameraPos = glm::vec3(0.0f));
            
    // Frees all OpenGL resources (VAO, VBO, shaders, textures)
    void cleanup();
    
    // Renders the model from the light's perspective to generate the shadow map depth texture
    void renderDepth(GLuint depthShader, const dlfdm::FDMSolver& fdm, const glm::mat4& lightSpaceMatrix);

private:
    // Standard vertex structure for 3D rendering
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    // OpenGL object handles
    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    GLuint shaderProgram_;
    GLuint texture_;
    
    // CPU-side mesh data
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    std::string texturePath_;

    // Internal helper functions
    bool loadObj(const std::string& objPath);
    bool loadTexture(const std::string& texturePath);
    bool createShaderProgram();
    GLuint compileShader(GLenum type, const char* source);
    bool finalizeMesh();
};

} // namespace Airplane