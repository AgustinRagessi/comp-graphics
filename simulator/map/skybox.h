#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Map {

/**
 * @brief Skybox renderer using cubemap textures
 * Renders a cube with inward-facing normals for viewing from inside
 */
class Skybox {
public:
    Skybox();
    ~Skybox();

    // No copying
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    /**
     * @brief Initialize skybox with texture faces
     * @param faces Vector of 6 texture paths in order: right, left, top, bottom, front, back
     * @return true if initialization succeeded
     */
    bool initialize(const std::vector<std::string>& faces);

    /**
     * @brief Render the skybox
     * @param view View matrix (translation will be removed internally)
     * @param projection Projection matrix
     */
    void render(const glm::mat4& view, const glm::mat4& projection);

    void cleanup();

private:
    GLuint vao_;
    GLuint vbo_;
    GLuint cubemap_texture_;
    GLuint shader_program_;

    bool setupGeometry();
    bool createShaderProgram();
    GLuint loadCubemap(const std::vector<std::string>& faces);
    GLuint compileShader(GLenum type, const char* source);
};

} // namespace Map
