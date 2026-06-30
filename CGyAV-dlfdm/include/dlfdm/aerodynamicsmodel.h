#ifndef AERODYNAMICSMODEL_H
#define AERODYNAMICSMODEL_H

#include <ostream>

#include <glm/glm.hpp>

#include <dlfdm/defines.h>

namespace dlfdm {

class AerodynamicsModel
{
public:
    struct AeroDynamicForces {
        glm::vec3 body_forces;       /// [N] - Body frame
        glm::vec3 body_moments;      /// [N·m] - Body frame - [x=L, y=M, z=N]
    };

    AerodynamicsModel(const AircraftParameters& p);

    // Calculate angle of attack and sideslip from velocity
    void calculate_angles(const glm::vec3& vel, float& alpha, float& beta) const;

    // Calculate aerodynamic forces and moments
    AeroDynamicForces calculate(const glm::vec3& body_velocity,
                                const glm::vec3& body_omega,
                                const ControlInputs& controls);

    void log_all_titles(std::ostream& os, const char& sep = ',') const;
    void log_all(std::ostream& os, const char& sep = ',') const;

    void log_angles_titles(std::ostream& os, const char& sep = ',') const;
    void log_angles(std::ostream& os, const char& sep = ',') const;
    void log_forces_titles(std::ostream& os, const char& sep = ',') const;
    void log_forces(std::ostream& os, const char& sep = ',') const;
    void log_moments_titles(std::ostream& os, const char& sep = ',') const;
    void log_moments(std::ostream& os, const char& sep = ',') const;

private:
    const AircraftParameters& aircraft_data_;
    const float rho = 1.225f;  // Air density sea level [kg/m^3]

    glm::vec3 wind_forces_;         // Forces in wind axis
    glm::vec3 aero_moments_;
    glm::vec2 aero_angles_;

    glm::vec3 body_forces_;
    glm::vec3 body_moments_;
};

} // namespace dlfdm

#endif // AERODYNAMICSMODEL_H
