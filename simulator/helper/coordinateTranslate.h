#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Helper {

inline glm::mat4 nedToWorldBasis() {
    glm::mat4 basis(1.0f);
    basis[0] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f); // north -> -Z
    basis[1] = glm::vec4(1.0f, 0.0f,  0.0f, 0.0f); // east  -> +X
    basis[2] = glm::vec4(0.0f,-1.0f,  0.0f, 0.0f); // down  -> -Y
    basis[3] = glm::vec4(0.0f, 0.0f,  0.0f, 1.0f);
    return basis;
}

// Convert aircraft NED coordinates to the simulator's world coordinates.
// NED:    x = north, y = east,  z = down
// World:  x = east,  y = up,    z = -north
inline glm::vec3 nedToWorldPosition(const glm::vec3& nedPosition) {
    return glm::vec3(
        nedPosition.y,
        -nedPosition.z,
        -nedPosition.x
    );
}

// Convert simulator world coordinates back to NED.
inline glm::vec3 worldToNedPosition(const glm::vec3& worldPosition) {
    return glm::vec3(
        -worldPosition.z,
        worldPosition.x,
        -worldPosition.y
    );
}

// Direction vectors use the same axis mapping as positions, but without any
// notion of origin.
inline glm::vec3 nedToWorldDirection(const glm::vec3& nedDirection) {
    return glm::vec3(
        nedDirection.y,
        -nedDirection.z,
        -nedDirection.x
    );
}

inline glm::vec3 worldToNedDirection(const glm::vec3& worldDirection) {
    return glm::vec3(
        -worldDirection.z,
        worldDirection.x,
        -worldDirection.y
    );
}

inline glm::mat4 nedToWorldTransform(const glm::mat4& nedTransform) {
    const glm::mat4 basis = nedToWorldBasis();
    return basis * nedTransform * glm::transpose(basis);
}

} // namespace Helper
