#include "horizon.h"
#include <glm/gtc/matrix_transform.hpp>

void ArtificialHorizon::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = width / 2.0f;
    float cy = height / 2.0f;

    // Render the static aircraft reticle (boresight) at the center of the screen
    renderer.drawLine(cx - 30, cy, cx - 10, cy);
    renderer.drawLine(cx + 10, cy, cx + 30, cy);
    renderer.drawLine(cx, cy, cx, cy - 10);

    // Coordinate Transformation: Pitch Ladder
    // We create a model matrix anchored to the screen center, then apply the aircraft's roll.
    // By drawing the pitch ladder relative to this rotated matrix, the entire ladder banks 
    // seamlessly with the aircraft while translating up and down based on pitch.
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(cx, cy, 0.0f));
    model = glm::rotate(model, glm::radians(data.roll), glm::vec3(0.0f, 0.0f, 1.0f));

    float pixelsPerDegree = 12.0f;
    int centerPitch = (int)data.pitch;
    
    // Normalize the starting pitch to the nearest 5-degree increment for rendering
    int startPitch = (centerPitch - 20) / 5 * 5; 

    for (int p = startPitch; p <= centerPitch + 20; p += 5) {
        float y = (p - data.pitch) * pixelsPerDegree;
        
        if (p == 0) { 
            // True Horizon Line
            renderer.drawLineTransformed(-200, y, -50, y, model);
            renderer.drawLineTransformed(50, y, 200, y, model);
        } else if (p > 0) { 
            // Positive Pitch (Sky): Solid lines with downward-pointing ticks
            renderer.drawLineTransformed(-60, y, 60, y, model);
            renderer.drawLineTransformed(-60, y, -60, y - 10, model);
            renderer.drawLineTransformed(60, y, 60, y - 10, model);
            
            // Transform text positions to ensure numbers rotate with the ladder
            glm::vec4 textPosRight = model * glm::vec4(70, y - 8, 0.0f, 1.0f);
            renderer.drawNumber(p, textPosRight.x, textPosRight.y, 0.6f);
            glm::vec4 textPosLeft = model * glm::vec4(-90, y - 8, 0.0f, 1.0f);
            renderer.drawNumber(p, textPosLeft.x, textPosLeft.y, 0.6f);
            
        } else { 
            // Negative Pitch (Ground): Gapped lines with upward-pointing ticks
            renderer.drawLineTransformed(-60, y, -30, y, model);
            renderer.drawLineTransformed(30, y, 60, y, model);
            renderer.drawLineTransformed(-60, y, -60, y + 10, model);
            renderer.drawLineTransformed(60, y, 60, y + 10, model);
            
            glm::vec4 textPosRight = model * glm::vec4(70, y - 8, 0.0f, 1.0f);
            renderer.drawNumber(-p, textPosRight.x, textPosRight.y, 0.6f);
            glm::vec4 textPosLeft = model * glm::vec4(-90, y - 8, 0.0f, 1.0f);
            renderer.drawNumber(-p, textPosLeft.x, textPosLeft.y, 0.6f);
        }
    }
}