#pragma once
#include <glm/glm.hpp>
#include <embree4/rtcore.h>

class Ray 
{
public:
    glm::vec3 base;
    glm::vec3 direction;
    float near = 0.000001f;
    float far = std::numeric_limits<float>::infinity();
};