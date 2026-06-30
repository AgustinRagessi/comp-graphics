#include "airspeed.h"

void AirspeedIndicator::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = 200.0f; // Anchor horizontally to the left side
    float cy = height / 2.0f; // Anchor vertically to the center

    // Draw main vertical tape backbone
    renderer.drawLine(cx, cy - 200, cx, cy + 200);
    
    // Draw central reading pointer box (a static box in the center of the screen)
    renderer.drawLine(cx, cy, cx + 15, cy + 15);
    renderer.drawLine(cx, cy, cx + 15, cy - 15);
    renderer.drawLine(cx + 15, cy + 15, cx + 70, cy + 15);
    renderer.drawLine(cx + 15, cy - 15, cx + 70, cy - 15);
    renderer.drawLine(cx + 70, cy + 15, cx + 70, cy - 15);

    // Print exact current Speed Value inside the central box
    renderer.drawNumber((int)data.speed, cx + 15, cy - 8, 0.8f);

    // Complex Process: Draw scrolling background ticks (10 knots per tick)
    // We calculate a rolling window starting 50 knots below current speed and ending 50 knots above.
    float pixelsPerKnot = 4.0f;
    int currentSpeed = (int)data.speed;
    int startSpeed = (currentSpeed - 50) / 10 * 10; // Floor to the nearest 10

    for (int s = startSpeed; s <= currentSpeed + 50; s += 10) {
        if (s < 0) continue; // Speed cannot be negative
        
        // Calculate Y position based on the difference between the tick value and the actual float speed
        float y = cy + (s - data.speed) * pixelsPerKnot;
        
        // Only draw if within the visual bounds of the tape (clipping)
        if (y > cy - 200 && y < cy + 200) {
            renderer.drawLine(cx - 15, y, cx, y);
            renderer.drawNumber(s, cx - 60, y - 5, 0.6f); 
        }
    }
}