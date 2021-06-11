#include <utils.hpp>
#include "EvCamera.h"

glm::mat4 EvCamera::getVPMatrix(float aspectRatio) const {
    auto viewMatrix = glm::lookAt(glm::vec3(0,0,0), getViewDir(), glm::vec3(0,1,0));
    auto projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.01f, 100.0f);
    return glm::translate(projectionMatrix * viewMatrix, -position);
}

void EvCamera::handleInput(const EvInputHelper &input) {
    const float MOVE_SPEED = 0.08f;
    const float TURN_SPEED = 0.04f;

    auto viewDir = getViewDir();
    auto sideWays = glm::normalize(glm::cross(viewDir, glm::vec3(0,1,0)));
    if (input.isDown(GLFW_KEY_W)) position += viewDir * MOVE_SPEED;
    if (input.isDown(GLFW_KEY_S)) position -= viewDir * MOVE_SPEED;
    if (input.isDown(GLFW_KEY_D)) position += sideWays * MOVE_SPEED;
    if (input.isDown(GLFW_KEY_A)) position -= sideWays * MOVE_SPEED;

    if (input.isDown(GLFW_KEY_UP)) phi = std::clamp(phi + TURN_SPEED, 0.01f, 3.1415926f);
    if (input.isDown(GLFW_KEY_DOWN)) phi = std::clamp(phi - TURN_SPEED, 0.01f, 3.1415926f);
    if (input.isDown(GLFW_KEY_LEFT)) theta -= TURN_SPEED;
    if (input.isDown(GLFW_KEY_RIGHT)) theta += TURN_SPEED;
}