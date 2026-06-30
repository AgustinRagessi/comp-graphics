#include "altimeter.h"

void Altimeter::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = width - 200.0f; // Fixed to the right side
    float cy = height / 2.0f;

    // Draw main vertical tape
    renderer.drawLine(cx, cy - 200, cx, cy + 200);
    
    // Draw central reading pointer box
    renderer.drawLine(cx, cy, cx - 15, cy + 15);
    renderer.drawLine(cx, cy, cx - 15, cy - 15);
    renderer.drawLine(cx - 15, cy + 15, cx - 90, cy + 15); 
    renderer.drawLine(cx - 15, cy - 15, cx - 90, cy - 15);
    renderer.drawLine(cx - 90, cy + 15, cx - 90, cy - 15);

    // Current Altitude Value
    renderer.drawNumber((int)data.altitude, cx - 85, cy - 8, 0.8f);

    // Draw scrolling ticks (100 ft per tick)
    float pixelsPerFt = 0.5f; 
    int currentAlt = (int)data.altitude;
    int startAlt = (currentAlt - 500) / 100 * 100;

    for (int a = startAlt; a <= currentAlt + 500; a += 100) {
        float y = cy + (a - data.altitude) * pixelsPerFt;
        
        if (y > cy - 200 && y < cy + 200) {
            renderer.drawLine(cx, y, cx + 15, y); 
            // Altitude Tick Value
            renderer.drawNumber(a, cx + 25, y - 5, 0.6f);
            
            // Draw a minor 50ft tick
            float minorY = y + (50 * pixelsPerFt);
            if (minorY < cy + 200) renderer.drawLine(cx, minorY, cx + 8, minorY);
        }
    }
}