#include <Core/Scene/Light/TriangleLightSource/TriangleLightSource.hpp>

TriangleLightSource::TriangleLightSource(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    points[0] = a;
    points[1] = b;
    points[2] = c;
    auto crss = glm::cross(points[1] - points[0], points[2] - points[0]);
    normal = glm::normalize(crss);
    pdf = glm::length(crss) / 2.0f;
}

glm::vec3 TriangleLightSource::sample_random_point() {
    float r1 = static_cast<float>(rand()) / RAND_MAX;
    float r2 = static_cast<float>(rand()) / RAND_MAX;
    float sqrt_r1 = std::sqrt(r1);
    float u = 1.0f - sqrt_r1;
    float v = r2 * sqrt_r1;
    return points[0] * (1.0f - sqrt_r1) + points[1] * (sqrt_r1 * (1.0f - r2)) + points[2] * (r2 * sqrt_r1);
}

// Solid Angle PDF
FloatColor TriangleLightSource::get_effective_color(float r, float cos_theta_i, float cos_theta_l) {
    return emissive_color * ((strength * cos_theta_i * cos_theta_l) / (pdf * r * r));
}