#include "compass.h"

void Compass::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = width / 2.0f; // Anchor horizontally to the center
    float cy = height - 100.0f; // Anchor vertically to the top of the screen

    // Main horizontal tape backbone
    renderer.drawLine(cx - 200, cy, cx + 200, cy);
    
    // Center heading pointer triangle
    renderer.drawLine(cx, cy, cx - 10, cy + 15);
    renderer.drawLine(cx, cy, cx + 10, cy + 15);
    renderer.drawLine(cx - 10, cy + 15, cx + 10, cy + 15);
    
    // Print exact Current Heading Value
    renderer.drawNumber((int)data.heading, cx - 10, cy + 20, 0.6f);

    // Complex Process: Draw scrolling ticks along the X axis
    float pixelsPerDegree = 8.0f;
    int currentHeading = (int)data.heading;
    int startHeading = currentHeading - 30; // 60 degree viewport window

    for (int h = startHeading; h <= currentHeading + 30; h++) {
        if (h % 5 == 0) { // Only calculate for every 5 degrees
            float offset = (h - data.heading);
            
            // Normalize display heading for 0-360 wraparound 
            // (e.g., if we are pointing North (0), we still need to draw 350 to the left)
            int displayHeading = h;
            if (displayHeading <= 0) displayHeading += 360;
            if (displayHeading > 360) displayHeading -= 360;
            
            float x = cx + (offset * pixelsPerDegree);
            
            // Only draw if within the visual bounds of the tape
            if (x > cx - 200 && x < cx + 200) {
                if (h % 10 == 0) {
                    renderer.drawLine(x, cy, x, cy - 15); // Major 10-degree tick
                    // Display heading divided by 10 (aviation standard, e.g. 120 -> 12)
                    renderer.drawNumber(displayHeading / 10, x - 10, cy - 35, 0.6f);
                } else {
                    renderer.drawLine(x, cy, x, cy - 8);  // Minor 5-degree tick
                }
            }
        }
    }
}