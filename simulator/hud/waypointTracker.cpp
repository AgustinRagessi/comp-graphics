#include "waypointTracker.h"
#include <cmath>

void WaypointTracker::render(const hud::FlightData& data, HudManager& renderer, int width, int height) {
    float cx = width / 2.0f;
    float cy = height - 100.0f; 

    // Navigation Math: Calculate absolute heading toward the waypoint
    float dx = data.waypoint.latitude - data.aircraft_x; 
    float dy = data.waypoint.longitud - data.aircraft_y; 
    
    // In a NED coordinate system, X aligns with North (0 degrees) and Y with East (90 degrees).
    // Using atan2(Y, X) directly yields the correct compass bearing in radians.
    float targetHeading = atan2(dy, dx) * (180.0f / 3.14159f);
    
    // Calculate the difference between where the nose is pointing and where the waypoint is.
    float bearingError = targetHeading - data.heading;
    
    // Normalize the error to a standard [-180, 180] degree range to determine the shortest turn direction.
    while (bearingError <= -180.0f) bearingError += 360.0f;
    while (bearingError > 180.0f) bearingError -= 360.0f;

    // Map the angular error to screen space matching the primary Compass scale.
    float pixelsPerDegree = 8.0f; 
    float x = cx + (bearingError * pixelsPerDegree);

    // Render Logic: On-screen vs. Off-screen clamping
    if (x > cx - 200 && x < cx + 200) {
        // If the waypoint lies within the forward field of view, render it as a diamond
        // attached beneath the compass tape.
        renderer.drawLine(x, cy - 20, x - 10, cy - 30);
        renderer.drawLine(x - 10, cy - 30, x, cy - 40);
        renderer.drawLine(x, cy - 40, x + 10, cy - 30);
        renderer.drawLine(x + 10, cy - 30, x, cy - 20);
        
        renderer.drawLine(x, cy - 20, x, cy - 5);
    } else {
        // If the waypoint is outside the compass viewport, clamp the indicator to the edge 
        // of the tape and render it as a directional arrow suggesting which way to turn.
        float edgeX = (bearingError > 0) ? cx + 200 : cx - 200;
        float dir = (bearingError > 0) ? -10.0f : 10.0f;
        
        renderer.drawLine(edgeX, cy - 30, edgeX + dir, cy - 20);
        renderer.drawLine(edgeX, cy - 30, edgeX + dir, cy - 40);
        renderer.drawLine(edgeX + dir, cy - 20, edgeX + dir, cy - 40);
    }
}