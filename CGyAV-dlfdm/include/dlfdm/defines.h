#ifndef FDM_DEFINES_H
#define FDM_DEFINES_H

#pragma once
#include <glm/glm.hpp>

namespace dlfdm {

struct ControlInputs {
    float throttle;      // [0, 1]
    float elevator;      // [rad] - pitch control
    float aileron;       // [rad] - roll control
    float rudder;        // [rad] - yaw control
};

struct AircraftState {
    glm::vec3 intertial_position;       // [m] - [x=north, y=east, z=down] Position (Inertial frame)
    glm::vec3 boby_velocity;            // [m/s] - [x=u, y=v, z=w] Velocity (Body frame)
    glm::vec3 body_omega;               // [rad/s] - [x=p, y=q, z=r] Angular velocity (Body frame)

    // Attitude (Euler angles)
    float phi;                          // [rad] - Roll angle
    float theta;                        // [rad] - Pitch angle
    float psi;                          // [rad] - Yaw angle psi
};

struct AircraftParameters {
    // Mass properties
    float mass;             // [kg]
    float Ixx, Iyy, Izz;    // [kg·m2] Moments of inertia
    float Ixz;              // [kg·m2] Cross moment

    // Aerodynamic reference data
    float wingArea;
    float wingChord;
    float wingSpan;

    // Propulsion
    float maxThrust;       // Maximum thrust at sea level

    // Aerodynamic coefficients (linear model)
    float CL0, CLa, CL_delta_e;         // Lift: CL = CL0 + CLa*alpha + CL_delta_e*delta_e
    float CD0, CDa;                     // Drag: CD = CD0 + CDa*alpha
    float Cm0, Cma, Cm_q;               // Pitch moment: Cm = Cm0 + Cma*alpha + Cmq*q*c_bar/2V
    float CY_beta, CY_r, CY_delta_r;    // Side: CY = CY_beta*beta + CY_r*r + CY_delta_r*delta_r
    float Cl_beta;                      // Roll moment from sideslip
    float Cn_beta;                      // Yaw moment from sideslip
    float Cl_p, Cn_r, Cl_r, Cn_p;       // Lateral damping coefficients

    // Control effectiveness      
    float Cm_delta_e;      // Pitch control
    float Cl_delta_a;      // Roll control
    float Cn_delta_r;      // Yaw control

    // Max control deflections
    float min_elevator;     // [rad]
    float max_elevator;     // [rad]
    float min_aileron;      // [rad]
    float max_aileron;      // [rad]
    float max_rudder;       // [rad]
};

}   // End namespace dlfdm

#endif // FDM_DEFINES_H
