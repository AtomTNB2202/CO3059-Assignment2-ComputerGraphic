#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 Camera::position() const
{
    float horizontal = glm::radians(yaw), vertical = glm::radians(pitch);
    return target + distance * glm::vec3(std::cos(vertical) * std::sin(horizontal), std::sin(vertical),
                                         std::cos(vertical) * std::cos(horizontal));
}
glm::mat4 Camera::view() const
{
    return glm::lookAt(position(), target, {0, 1, 0});
}
glm::mat4 Camera::projection(float aspect) const
{
    return glm::perspective(glm::radians(verticalFovDegrees()), aspect, nearPlane(), farPlane());
}
float Camera::farPlane() const { return std::max(500.0f, distance * 10); }
void Camera::orbit(float dx, float dy)
{
    frontView = false;
    yaw -= dx * 0.3f;
    pitch = std::clamp(pitch + dy * 0.3f, -89.0f, 89.0f);
}
void Camera::pan(float dx, float dy, float height)
{
    target -= screenDelta(dx, dy, distance, height);
}

glm::vec3 Camera::screenDelta(float dx, float dy, float depth, float height) const
{
    auto forward = glm::normalize(target - position());
    auto right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    auto up = glm::cross(right, forward);
    float units = 2 * depth * std::tan(glm::radians(22.5f)) / std::max(height, 1.0f);
    return (dx * right - dy * up) * units;
}

void Camera::move(glm::vec3 direction, float seconds)
{
    auto forward = glm::normalize(target - position());
    auto right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    auto movement = direction.x * right + direction.y * glm::vec3(0, 1, 0) + direction.z * forward;
    float length = glm::length(movement);
    if (length > 0.0001f)
        target += movement / length * moveSpeed * std::clamp(seconds, 0.0f, 0.1f);
}
void Camera::zoom(float amount)
{
    distance = std::clamp(distance * std::exp(-amount * 0.12f), 0.3f, 100000.0f);
}
void Camera::front()
{
    frontView = true;
    yaw = 0;
    pitch = 0;
}
void Camera::reset()
{
    frontView = false;
    target = {0, 0, 0};
    yaw = 28;
    pitch = 20;
    distance = 7;
}

void Camera::frame(glm::vec3 low, glm::vec3 high, float aspect)
{
    target = (low + high) * 0.5f;
    float radius = std::max(glm::length(high - low) * 0.5f, 0.05f);
    float halfFov = std::atan(std::tan(glm::radians(22.5f)) * std::min(aspect, 1.0f));
    distance = std::clamp(radius / std::sin(halfFov) * 1.15f, 0.3f, 100000.0f);
}
