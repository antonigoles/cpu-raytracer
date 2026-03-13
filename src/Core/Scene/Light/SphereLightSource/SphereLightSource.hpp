#pragma once
#include <Core/FloatColor/FloatColor.hpp>
#include <Core/Color/Color.hpp>
#include <glm/glm.hpp>

class SphereLightSource
{
public:
    glm::vec3 center;
    float radius;
    FloatColor emissive_color = Color(255, 255, 255, 255).as_floats();
    float strength = 10.0f;

    SphereLightSource(glm::vec3 center, float radius, FloatColor emissive_color, float strength);
    glm::vec3 get_closest_point(glm::vec3 from);

    FloatColor get_effective_color(float r, float cos_theta);
};