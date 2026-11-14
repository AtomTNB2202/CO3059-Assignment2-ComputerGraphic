#pragma once
#include <glm/glm.hpp>

class Camera
{
  public:
    glm::vec3 target{0};
    float yaw = 28, pitch = 20, distance = 7;
    float moveSpeed = 5;
    bool frontView = false;
    glm::vec3 position() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    float nearPlane() const { return 0.05f; }
    float farPlane() const;
    float verticalFovDegrees() const { return 45.0f; }
    void orbit(float dx, float dy);
    void pan(float dx, float dy, float height);
    void zoom(float amount);
    void move(glm::vec3 direction, float seconds);
    glm::vec3 screenDelta(float dx, float dy, float depth, float height) const;
    void front();
    void reset();
    void frame(glm::vec3 low, glm::vec3 high, float aspect);
};
