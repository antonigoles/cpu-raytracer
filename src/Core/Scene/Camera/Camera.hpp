#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera 
{
public:
    Camera();

    float fov;

    glm::vec3 position;
    glm::vec3 up;

    glm::quat rotation;

    glm::vec3 get_forward();

    glm::vec3 get_right();

    glm::vec3 get_up();
};