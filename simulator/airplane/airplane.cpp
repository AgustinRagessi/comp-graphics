#include "airplane.h"

#include <dlfdm/aircraftdynamics.h>
#include <dlfdm/defines.h>
#include <dlfdm/fdmsolver.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "../helper/coordinateTranslate.h"

namespace Airplane {

static const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D airplaneTexture;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 cameraPos;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
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
    vec3 baseColor = texture(airplaneTexture, TexCoord).rgb;
    
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    
    vec3 ambient = ambientColor * baseColor;
    
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * baseColor;
    
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = 0.7 * spec * lightColor; 
    
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDirection);
    
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    FragColor = vec4(result, 1.0);
}
)";

AirplaneModel::AirplaneModel()
    : vao_(0), vbo_(0), ebo_(0), shaderProgram_(0), texture_(0) {}

AirplaneModel::~AirplaneModel() {
    cleanup();
}

bool AirplaneModel::initialize(const std::string& objPath) {
    if (!loadObj(objPath)) return false;
    if (!finalizeMesh()) return false;
    if (!createShaderProgram()) return false;

    // Attempt to load the primary texture. If it fails, create a procedural 1x1 gray pixel fallback texture
    // so the shader doesn't crash or render completely black.
    if (!loadTexture("textures/airplane/rusty_metal.jpg")) {
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        unsigned char fallbackPixel[] = {150, 150, 150, 255}; 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    return true;
}

// Complex Process: Building the model matrix from Flight Dynamics (NED)
glm::mat4 buildAirplaneModelMatrix(const dlfdm::AircraftState& state) {
    // 1. Translate intertial position from NED to OpenGL World coordinates
    glm::vec3 worldPosition = Helper::nedToWorldPosition(state.intertial_position);
    glm::mat4 model(1.0f);
    model = glm::translate(model, worldPosition);

    // 2. Build rotation matrix using aircraft Euler angles (Yaw, Pitch, Roll) in NED space
    glm::mat4 rotNed(1.0f);
    rotNed = glm::rotate(rotNed, state.psi, glm::vec3(0.0f, 0.0f, 1.0f));   // Yaw
    rotNed = glm::rotate(rotNed, state.theta, glm::vec3(0.0f, 1.0f, 0.0f)); // Pitch
    rotNed = glm::rotate(rotNed, state.phi, glm::vec3(1.0f, 0.0f, 0.0f));   // Roll

    // 3. Extract Forward, Right, and Down directional vectors from the NED rotation matrix
    glm::vec3 fwdNed = glm::vec3(rotNed * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    glm::vec3 rightNed = glm::vec3(rotNed * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    glm::vec3 downNed = glm::vec3(rotNed * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

    // 4. Convert NED directional vectors to standard World directional vectors
    glm::vec3 fwdWorld = Helper::nedToWorldDirection(fwdNed);
    glm::vec3 rightWorld = Helper::nedToWorldDirection(rightNed);
    glm::vec3 upWorld = Helper::nedToWorldDirection(-downNed); // Up is negative Down

    // 5. Construct a new orientation matrix natively in OpenGL space using the converted basis vectors
    glm::mat4 orientation(1.0f);
    orientation[0] = glm::vec4(rightWorld, 0.0f);
    orientation[1] = glm::vec4(upWorld, 0.0f);
    orientation[2] = glm::vec4(-fwdWorld, 0.0f); // -Z is forward in OpenGL
    orientation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Apply final transformations: orientation, a structural 90-degree fix for the 3D model, and scale.
    model = model * orientation;
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
    
    return model;
}

void AirplaneModel::renderDepth(GLuint depthShader, const dlfdm::FDMSolver& fdm, const glm::mat4& lightSpaceMatrix) {
    if (!vao_) return;
    
    glm::mat4 model = buildAirplaneModelMatrix(fdm.getState());
    
    glUseProgram(depthShader);
    glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void AirplaneModel::render(const dlfdm::FDMSolver& fdm,
                           const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::mat4& lightSpaceMatrix,
                           GLuint shadowMap,
                           const glm::vec3& lightDir,
                           const glm::vec3& cameraPos) {
    if (!shaderProgram_ || !vao_) return;

    glm::mat4 model = buildAirplaneModelMatrix(fdm.getState());

    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    glUniform3fv(glGetUniformLocation(shaderProgram_, "lightDir"), 1, glm::value_ptr(glm::normalize(lightDir)));
    glUniform3f(glGetUniformLocation(shaderProgram_, "lightColor"), 1.0f, 0.95f, 0.9f);
    glUniform3f(glGetUniformLocation(shaderProgram_, "ambientColor"), 0.35f, 0.35f, 0.35f);
    glUniform3fv(glGetUniformLocation(shaderProgram_, "cameraPos"), 1, glm::value_ptr(cameraPos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "airplaneTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glUniform1i(glGetUniformLocation(shaderProgram_, "shadowMap"), 1);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    glActiveTexture(GL_TEXTURE0); 
}

void AirplaneModel::cleanup() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (texture_) glDeleteTextures(1, &texture_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);

    vao_ = 0;
    vbo_ = 0;
    ebo_ = 0;
    texture_ = 0;
    shaderProgram_ = 0;
}

bool AirplaneModel::loadObj(const std::string& objPath) {
    std::ifstream file(objPath);
    if (!file.is_open()) {
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    vertices_.clear();
    indices_.clear();

    // Complex Process: Parsing OBJ files and deduplicating vertices
    // OBJ files define vertices, normals, and uvs separately, and faces combine them via indices (v/vt/vn).
    // OpenGL requires a single unified Vertex array. We use a Hash Map to merge unique v/vt/vn combinations
    // into single Vertex objects and generate an EBO (Element Buffer Object) index array.
    struct KeyHash {
        std::size_t operator()(const std::tuple<int, int, int>& key) const {
            return std::hash<int>()(std::get<0>(key)) ^ (std::hash<int>()(std::get<1>(key)) << 1) ^ (std::hash<int>()(std::get<2>(key)) << 2);
        }
    };

    std::unordered_map<std::tuple<int, int, int>, unsigned int, KeyHash> vertexMap;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (type == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            texcoords.push_back(uv);
        } else if (type == "f") {
            std::vector<std::string> faceVertices;
            std::string faceVertex;
            while (ss >> faceVertex) {
                faceVertices.push_back(faceVertex);
            }

            auto parseIndex = [](const std::string& token, int& v, int& t, int& n) {
                v = t = n = 0;
                std::stringstream tokenStream(token);
                std::string item;
                std::getline(tokenStream, item, '/');
                v = std::stoi(item);
                if (std::getline(tokenStream, item, '/')) {
                    if (!item.empty()) t = std::stoi(item);
                }
                if (std::getline(tokenStream, item, '/')) {
                    if (!item.empty()) n = std::stoi(item);
                }
            };

            // Loop through face vertices (triangulating polygons using fan method if > 3 vertices)
            for (std::size_t i = 1; i + 1 < faceVertices.size(); ++i) {
                const std::string tri[3] = {faceVertices[0], faceVertices[i], faceVertices[i + 1]};
                for (const std::string& token : tri) {
                    int vi, ti, ni;
                    parseIndex(token, vi, ti, ni);
                    auto key = std::make_tuple(vi, ti, ni);
                    auto found = vertexMap.find(key);

                    // If this exact combination of v/vt/vn hasn't been seen yet, create it.
                    if (found == vertexMap.end()) {
                        Vertex vertex{};
                        // OBJ indices are 1-based, C++ arrays are 0-based. Subtract 1.
                        vertex.position = positions.at(static_cast<std::size_t>(vi - 1));
                        vertex.texcoord = (ti > 0 && static_cast<std::size_t>(ti - 1) < texcoords.size()) ? texcoords.at(static_cast<std::size_t>(ti - 1)) : glm::vec2(0.0f);
                        vertex.normal = (ni > 0 && static_cast<std::size_t>(ni - 1) < normals.size()) ? normals.at(static_cast<std::size_t>(ni - 1)) : glm::vec3(0.0f, 1.0f, 0.0f);
                        const unsigned int newIndex = static_cast<unsigned int>(vertices_.size());
                        vertices_.push_back(vertex);
                        indices_.push_back(newIndex);
                        vertexMap[key] = newIndex;
                    } else {
                        // If we already have this vertex combination, just reuse its index.
                        indices_.push_back(found->second);
                    }
                }
            }
        }
    }

    return !vertices_.empty() && !indices_.empty();
}

bool AirplaneModel::finalizeMesh() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texcoord)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return true;
}

bool AirplaneModel::createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vertexShader || !fragmentShader) {
        return false;
    }

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);

    GLint success = 0;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "Airplane shader linking failed:\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

GLuint AirplaneModel::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Airplane shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }

    return shader;
}

bool AirplaneModel::loadTexture(const std::string& texturePath) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        return false;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return true;
}

} // namespace Airplane