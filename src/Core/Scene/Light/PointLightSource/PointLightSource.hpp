#pragma once
#include <Core/FloatColor/FloatColor.hpp>
#include <glm/glm.hpp>

class PointLightSource
{
public:
    glm::vec3 position;
    FloatColor color;
    float strength;

    FloatColor get_effective_color(float r, float cos_theta);
};