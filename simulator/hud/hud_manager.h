#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "hud_data.h"

class HudManager {
public:
    HudManager();
    ~HudManager();

    bool initialize();
    void beginRender(int screenWidth, int screenHeight);
    void drawLine(float x1, float y1, float x2, float y2);
    void drawLineTransformed(float x1, float y1, float x2, float y2, const glm::mat4& transform);
    
    // NEW: Draw a whole number
    void drawNumber(int value, float x, float y, float scale = 1.0f); 

    void endRender();

private:
    struct HudVertex {
        float x, y;
        float r, g, b;
    };

    GLuint vao_, vbo_, shaderProgram_;
    std::vector<HudVertex> lineVertices_;
    glm::mat4 projection_;
    glm::vec3 currentColor_;

    GLuint compileShader(GLenum type, const char* source);
    
    // Helper to draw a single 7-segment digit
    void drawDigit(int digit, float x, float y, float scale); 
};