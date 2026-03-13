#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera 
{
public:
    Camera();

    float fov;

    glm::vec3 position;
    glm::quat rotation;

    glm::vec3 forward();

    glm::vec3 right();

    glm::vec3 up();
};