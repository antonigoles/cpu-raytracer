#include <Core/Scene/Camera/Camera.hpp>

Camera::Camera() {};

glm::vec3 Camera::forward()
{
    return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Camera::right()
{
    return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::up()
{
    return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
}