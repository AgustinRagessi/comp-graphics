#pragma once

#include <glm/glm.hpp>
#include <dlfdm/defines.h>

// Defines the three states the camera can be in
enum class CameraMode {
    CHASE,
    FRONT,
    FIRST_PERSON
};

class Camera {
public:
    Camera();

    // Updates the camera position based on the airplane's physics state
    void update(const dlfdm::AircraftState& state);
    
    // Fallback mode if the physics engine crashes
    void setSafeMode(const glm::vec3& safePos);
    
    // Cycles through CHASE -> FRONT -> FIRST_PERSON
    void toggleMode();

    // Matrix and data getters for the rendering pipeline
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const;
    CameraMode getMode() const { return mode_; }

private:
    CameraMode mode_;
    glm::vec3 position_;
    glm::vec3 target_;
    glm::vec3 up_;
};