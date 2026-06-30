#pragma once

namespace hud {

    // Defines a target geographical or Cartesian location for the navigation systems
    struct Waypoint {
        float latitude;     
        float longitud;     
        float altitude;     
    };

    // Centralized telemetry structure updated by the physics engine per frame
    struct FlightData {
        float pitch;           // [deg] Pitch angle (nose up/down)
        float roll;            // [deg] Roll angle (bank left/right)
        float heading;         // [deg] Yaw/Compass heading
        float altitude;        // [ft]  Above sea level or ground level
        float speed;           // [kt]  Indicated airspeed
        float vertical_speed;  // [ft/min] Rate of climb or descent
        
        // Current Cartesian coordinates for navigation math
        float aircraft_x;
        float aircraft_y;
        
        Waypoint waypoint;
    };

} 