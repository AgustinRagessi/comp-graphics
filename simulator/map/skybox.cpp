#include "skybox.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <iostream>
#include <cstdio>

namespace Map
{

    // Skybox cube vertices (36 vertices with inward-facing normals)
    static const float skyboxVertices[] = {
        // Back face
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        // Front face
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        // Left face
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,

        // Right face
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,

        // Top face
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        // Bottom face
        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f};

    // Skybox vertex shader source
    static const char *vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

    // Skybox fragment shader source
    static const char *fragmentShaderSource = R"(
#version 460 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}
)";

    Skybox::Skybox()
        : vao_(0), vbo_(0), cubemap_texture_(0), shader_program_(0) {}

    Skybox::~Skybox()
    {
        cleanup();
    }

    bool Skybox::initialize(const std::vector<std::string> &faces)
    {
        if (faces.size() != 6)
        {
            std::cerr << "Skybox requires exactly 6 texture faces" << std::endl;
            return false;
        }

        // Create shader program
        if (!createShaderProgram())
        {
            std::cerr << "Failed to create skybox shader program" << std::endl;
            return false;
        }

        // Setup geometry
        if (!setupGeometry())
        {
            std::cerr << "Failed to setup skybox geometry" << std::endl;
            return false;
        }

        // Load cubemap texture
        cubemap_texture_ = loadCubemap(faces);
        if (cubemap_texture_ == 0)
        {
            std::cerr << "Failed to load skybox cubemap" << std::endl;
            return false;
        }

        std::cout << "Skybox initialized successfully" << std::endl;
        return true;
    }

    void Skybox::render(const glm::mat4 &view, const glm::mat4 &projection)
    {
        // Structural Trick: Strip the translation (position) data from the view matrix.
        // By keeping only the 3x3 rotation matrix, the skybox rotates when the player looks around,
        // but the player can never actually move closer to the edges of the box.
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

        // Depth Trick: Change the depth function to GL_LEQUAL (Less than or Equal).
        // The skybox vertex shader sets `gl_Position = pos.xyww;` ensuring the skybox always has a maximum depth of 1.0.
        // GL_LEQUAL allows the skybox to render successfully at the absolute back of the depth buffer.
        glDepthFunc(GL_LEQUAL);
        glUseProgram(shader_program_);

        GLint viewLoc = glGetUniformLocation(shader_program_, "view");
        GLint projLoc = glGetUniformLocation(shader_program_, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Revert texture and depth settings to normal so subsequent UI/HUD rendering isn't corrupted
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    void Skybox::cleanup()
    {
        if (vao_)
            glDeleteVertexArrays(1, &vao_);
        if (vbo_)
            glDeleteBuffers(1, &vbo_);
        if (cubemap_texture_)
            glDeleteTextures(1, &cubemap_texture_);
        if (shader_program_)
            glDeleteProgram(shader_program_);

        vao_ = vbo_ = cubemap_texture_ = shader_program_ = 0;
    }

    bool Skybox::setupGeometry()
    {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        return true;
    }

    bool Skybox::createShaderProgram()
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        if (vertexShader == 0 || fragmentShader == 0)
        {
            return false;
        }

        shader_program_ = glCreateProgram();
        glAttachShader(shader_program_, vertexShader);
        glAttachShader(shader_program_, fragmentShader);
        glLinkProgram(shader_program_);

        int success;
        glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shader_program_, 512, nullptr, infoLog);
            std::cerr << "Shader program linking failed:\n"
                      << infoLog << std::endl;
            return false;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return true;
    }

    GLuint Skybox::loadCubemap(const std::vector<std::string> &faces)
    {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        // Flip BMP images vertically (they're often stored upside down)
        stbi_set_flip_vertically_on_load(false);

        std::cout << "\n=== Loading Cubemap Textures ===" << std::endl;
        for (GLuint i = 0; i < faces.size(); i++)
        {
            std::cout << "[" << i << "] Attempting to load: " << faces[i] << std::endl;

            // Check if file exists by attempting to open it
            FILE *file = fopen(faces[i].c_str(), "rb");
            if (!file)
            {
                std::cerr << "  ERROR: File does not exist or cannot be opened!" << std::endl;
                std::cerr << "  Path: " << faces[i] << std::endl;
                glDeleteTextures(1, &textureID);
                return 0;
            }
            fclose(file);

            int width, height, channels;
            unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

            if (data)
            {
                std::cout << "  SUCCESS: " << width << "x" << height << " with " << channels << " channels" << std::endl;
                GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                const char *reason = stbi_failure_reason();
                std::cerr << "  ERROR: stbi_load failed!" << std::endl;
                std::cerr << "  Reason: " << (reason ? reason : "Unknown") << std::endl;
                std::cerr << "  File: " << faces[i] << std::endl;
                glDeleteTextures(1, &textureID);
                return 0;
            }
        }
        std::cout << "=== All textures loaded successfully ===\n"
                  << std::endl;

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }

    GLuint Skybox::compileShader(GLenum type, const char *source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed:\n"
                      << infoLog << std::endl;
            return 0;
        }
        return shader;
    }

} // namespace Map
