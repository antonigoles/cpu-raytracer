#pragma once
#include <Core/FloatColor/FloatColor.hpp>
#include <Core/Color/Color.hpp>
#include <glm/glm.hpp>

class TriangleLightSource
{
public:
    glm::vec3 points[3];
    glm::vec3 normal;
    FloatColor emissive_color = Color(255, 255, 255, 255).as_floats();
    float strength = 10.0f;
    float pdf;

    TriangleLightSource(glm::vec3 a, glm::vec3 b, glm::vec3 c);

    glm::vec3 sample_random_point();

    // Solid Angle PDF
    FloatColor get_effective_color(float r, float cos_theta_i, float cos_theta_l);
};