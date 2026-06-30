#ifndef FDMSOLVER_H
#define FDMSOLVER_H

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <dlfdm/defines.h>
#include <dlfdm/aerodynamicsmodel.h>
#include <dlfdm/aircraftdynamics.h>

namespace dlfdm {

class FDMSolver
{
public:
    FDMSolver(const AircraftParameters& p, float dt = 1.0f / 120.0f);

    void update(const ControlInputs& controls);

    const AircraftState& getState() const { return aircraft_state_; }
    void setState(const AircraftState& newState) { aircraft_state_ = newState; }

    const AircraftDynamics::StateDerivatives get_state_dot() const {
        return state_deriv_;
    }

    void setTimeStep(float dt) { time_step_ = dt; }

    float get_sim_time(void) const      { return time_; }

    glm::mat4 getModelMatrix() const;

    void log_titles(std::ostream& os, const char& sep = ',') const;
    void log_state(std::ostream& os, const char& sep = ',') const;

    ///
    /// \brief get_aero_fm Return aerodynamic forces and moments
    /// \return Aerodynamic forces and moments
    ///
    AerodynamicsModel::AeroDynamicForces get_aero_fm(void) { return aero_fm_; }

private:
    AircraftState aircraft_state_;
    AircraftParameters aircraft_data_;
    AerodynamicsModel aerodynamics;
    AircraftDynamics dynamics;

    float time_step_;
    float time_;

    AerodynamicsModel::AeroDynamicForces aero_fm_;
    AircraftDynamics::StateDerivatives state_deriv_;

    void log_state_titles(std::ostream& os, const char& sep = ',') const;
    void log_aircraft_state(std::ostream& os, const char& sep = ',') const;
};

}   // End namespace dlfdm

#endif // FDMSOLVER_H
