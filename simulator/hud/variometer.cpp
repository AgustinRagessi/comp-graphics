#include "variometer.h"

void Variometer::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = width - 100.0f; 
    float cy = height / 2.0f;

    // Render the static structural backbone and tick marks for the VSI scale
    renderer.drawLine(cx, cy - 100, cx, cy + 100);
    renderer.drawLine(cx - 10, cy, cx, cy); 
    renderer.drawLine(cx - 10, cy + 50, cx, cy + 50); 
    renderer.drawLine(cx - 10, cy + 100, cx, cy + 100); 
    renderer.drawLine(cx - 10, cy - 50, cx, cy - 50); 
    renderer.drawLine(cx - 10, cy - 100, cx, cy - 100); 

    // Render the fixed labels (represented in Thousands of Feet Per Minute)
    renderer.drawNumber(0, cx + 5, cy - 8, 0.6f);
    renderer.drawNumber(1, cx + 5, cy + 42, 0.6f);
    renderer.drawNumber(2, cx + 5, cy + 92, 0.6f);
    renderer.drawNumber(-1, cx + 5, cy - 58, 0.6f);
    renderer.drawNumber(-2, cx + 5, cy - 108, 0.6f);

    // Constrain the visual needle so it does not render outside the physical scale
    float displayVSI = data.vertical_speed;
    if (displayVSI > 2000.0f) displayVSI = 2000.0f;
    if (displayVSI < -2000.0f) displayVSI = -2000.0f;

    // Calculate Y-offset: 50 pixels represent 1000 fpm (0.05 multiplier)
    float needleY = cy + (displayVSI * 0.05f); 
    
    renderer.drawLine(cx - 20, needleY, cx - 5, needleY + 8);
    renderer.drawLine(cx - 20, needleY, cx - 5, needleY - 8);
    renderer.drawLine(cx - 5, needleY + 8, cx - 5, needleY - 8);
}