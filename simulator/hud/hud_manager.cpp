#include "hud_manager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static const char* hudVS = R"(
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 projection;
void main() {
    vColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

static const char* hudFS = R"(
#version 460 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0); // No lighting, pure bright color
}
)";

HudManager::HudManager() : vao_(0), vbo_(0), shaderProgram_(0), currentColor_(0.1f, 0.9f, 0.2f) {} // HUD Green

HudManager::~HudManager() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
}

bool HudManager::initialize() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, hudVS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, hudFS);
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vs);
    glAttachShader(shaderProgram_, fs);
    glLinkProgram(shaderProgram_);
    
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    return true;
}

void HudManager::beginRender(int screenWidth, int screenHeight) {
    lineVertices_.clear();
    // Establish a 2D orthographic projection matching pixel dimensions.
    // Bottom-left is (0,0), Top-right is (screenWidth, screenHeight).
    projection_ = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
}
void HudManager::drawLine(float x1, float y1, float x2, float y2) {
    lineVertices_.push_back({x1, y1, currentColor_.r, currentColor_.g, currentColor_.b});
    lineVertices_.push_back({x2, y2, currentColor_.r, currentColor_.g, currentColor_.b});
}

void HudManager::drawLineTransformed(float x1, float y1, float x2, float y2, const glm::mat4& transform) {
    glm::vec4 p1 = transform * glm::vec4(x1, y1, 0.0f, 1.0f);
    glm::vec4 p2 = transform * glm::vec4(x2, y2, 0.0f, 1.0f);
    drawLine(p1.x, p1.y, p2.x, p2.y);
}

void HudManager::endRender() {
    if (lineVertices_.empty()) return;

    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, glm::value_ptr(projection_));

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Upload the batched line vertices to the GPU.
    // We use GL_DYNAMIC_DRAW because the HUD geometry is entirely rebuilt every single frame.
    glBufferData(GL_ARRAY_BUFFER, lineVertices_.size() * sizeof(HudVertex), lineVertices_.data(), GL_DYNAMIC_DRAW);
    
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineVertices_.size());
    glBindVertexArray(0);
}

GLuint HudManager::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

// ============================================================================
// VECTOR DIGITAL FONT (7-Segment Display)
// ============================================================================
void HudManager::drawDigit(int digit, float x, float y, float scale) {
    float w = 12.0f * scale; // Width of the digit
    float h = 20.0f * scale; // Height of the digit
    
    // 7 segments: 0:Top, 1:TopLeft, 2:TopRight, 3:Mid, 4:BotLeft, 5:BotRight, 6:Bot
    static const int segments[10][7] = {
        {1, 1, 1, 0, 1, 1, 1}, // 0
        {0, 0, 1, 0, 0, 1, 0}, // 1
        {1, 0, 1, 1, 1, 0, 1}, // 2
        {1, 0, 1, 1, 0, 1, 1}, // 3
        {0, 1, 1, 1, 0, 1, 0}, // 4
        {1, 1, 0, 1, 0, 1, 1}, // 5
        {1, 1, 0, 1, 1, 1, 1}, // 6
        {1, 0, 1, 0, 0, 1, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1}  // 9
    };

    if (digit < 0 || digit > 9) return;

    if (segments[digit][0]) drawLine(x, y+h, x+w, y+h);         // Top
    if (segments[digit][1]) drawLine(x, y+h, x, y+h/2.0f);      // Top-Left
    if (segments[digit][2]) drawLine(x+w, y+h, x+w, y+h/2.0f);  // Top-Right
    if (segments[digit][3]) drawLine(x, y+h/2.0f, x+w, y+h/2.0f); // Middle
    if (segments[digit][4]) drawLine(x, y+h/2.0f, x, y);        // Bot-Left
    if (segments[digit][5]) drawLine(x+w, y+h/2.0f, x+w, y);    // Bot-Right
    if (segments[digit][6]) drawLine(x, y, x+w, y);             // Bottom
}

void HudManager::drawNumber(int value, float x, float y, float scale) {
    if (value == 0) {
        drawDigit(0, x, y, scale);
        return;
    }
    
    bool negative = value < 0;
    if (negative) value = -value;

    // Mathematical Digit Extraction: 
    // Using modulo 10 isolates the rightmost digit. We then divide by 10 to shift the integer right.
    // This populates the vector in reverse order (e.g., 123 becomes {3, 2, 1}).
    std::vector<int> digits;
    while (value > 0) {
        digits.push_back(value % 10);
        value /= 10;
    }

    float w = 12.0f * scale;
    float spacing = 6.0f * scale;
    float startX = x;

    if (negative) {
        drawLine(startX, y + (10.0f * scale), startX + (8.0f * scale), y + (10.0f * scale)); 
        startX += (8.0f * scale) + spacing;
    }

    // Iterate through the vector in reverse to render the digits left-to-right correctly on screen.
    for (int i = digits.size() - 1; i >= 0; i--) {
        drawDigit(digits[i], startX, y, scale);
        startX += w + spacing;
    }
}