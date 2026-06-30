#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../helper/coordinateTranslate.h" 

Camera::Camera() 
    : mode_(CameraMode::CHASE), 
      position_(0.0f, 50.0f, 0.0f), 
      target_(0.0f, 0.0f, 0.0f), 
      up_(0.0f, 1.0f, 0.0f) {}

void Camera::toggleMode() {
    if (mode_ == CameraMode::CHASE) {
        mode_ = CameraMode::FRONT;
    } else if (mode_ == CameraMode::FRONT) {
        mode_ = CameraMode::FIRST_PERSON;
    } else {
        mode_ = CameraMode::CHASE;
    }
}

void Camera::setSafeMode(const glm::vec3& safePos) {
    position_ = safePos;
    target_ = glm::vec3(0.0f, 150.0f, 0.0f);
    up_ = glm::vec3(0.0f, 1.0f, 0.0f);
}

void Camera::update(const dlfdm::AircraftState& state) {
    // 1. Calculate the airplane's rotation matrix in NED space
    glm::mat4 rotNed(1.0f);
    rotNed = glm::rotate(rotNed, state.psi, glm::vec3(0.0f, 0.0f, 1.0f));
    rotNed = glm::rotate(rotNed, state.theta, glm::vec3(0.0f, 1.0f, 0.0f));
    rotNed = glm::rotate(rotNed, state.phi, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec3 offsetNed(0.0f);
    glm::vec3 lookAtOffsetNed(0.0f);

    // 2. Define relative offsets based on the current mode
    switch (mode_) {
        case CameraMode::CHASE:
            offsetNed = glm::vec3(-15.0f, 0.0f, -5.0f);     // Behind and above
            lookAtOffsetNed = glm::vec3(0.0f, 0.0f, 0.0f);  // Look directly at the plane
            break;
        case CameraMode::FRONT:
            offsetNed = glm::vec3(20.0f, 0.0f, -2.0f);      // In front of the nose
            lookAtOffsetNed = glm::vec3(0.0f, 0.0f, 0.0f);  // Look backward at the plane
            break;
        case CameraMode::FIRST_PERSON:
            offsetNed = glm::vec3(1.0f, 0.0f, -1.5f);       // Inside the cockpit canopy
            lookAtOffsetNed = glm::vec3(100.0f, 0.0f, -1.5f); // Look far out ahead of the plane
            break;
    }

    // 3. Apply the airplane's rotation to the offsets
    glm::vec3 rotatedOffsetNed = glm::vec3(rotNed * glm::vec4(offsetNed, 1.0f));
    glm::vec3 rotatedLookAtNed = glm::vec3(rotNed * glm::vec4(lookAtOffsetNed, 1.0f));

    // 4. Calculate final NED positions
    glm::vec3 camPosNed = state.intertial_position + rotatedOffsetNed;
    glm::vec3 targetPosNed = state.intertial_position + rotatedLookAtNed;

    // 5. Convert to OpenGL World Coordinates
    position_ = Helper::nedToWorldPosition(camPosNed);
    target_ = Helper::nedToWorldPosition(targetPosNed);

    // 6. Calculate the "Up" vector
    if (mode_ == CameraMode::FIRST_PERSON) {
        // In First-Person, the camera rolls with the airplane
        glm::vec3 upNed(0.0f, 0.0f, -1.0f); // In NED, 'Up' is negative Z
        glm::vec3 rotatedUpNed = glm::vec3(rotNed * glm::vec4(upNed, 0.0f));
        up_ = Helper::nedToWorldDirection(rotatedUpNed);
    } else {
        // In Chase/Front views, keep the camera locked to the horizon to prevent nausea
        up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position_, target_, up_);
}

glm::vec3 Camera::getPosition() const {
    return position_;
}