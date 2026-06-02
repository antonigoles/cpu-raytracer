#pragma once
#include <glm/glm.hpp>
#include <embree4/rtcore.h>

class Ray 
{
public:
    glm::vec3 base;
    glm::vec3 direction;
    float near = 0.000001f;
    float far = 99999999999999999999.0f;
    bool is_coherent = false;

    void move_base_by_direction(float strength) {
        this->base += direction * strength;
    };
};