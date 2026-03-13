#include <Core/Scene/Light/SphereLightSource/SphereLightSource.hpp>

SphereLightSource::SphereLightSource(glm::vec3 center, float radius, FloatColor emissive_color, float strength) 
: center(center), radius(radius), emissive_color(emissive_color), strength(strength) {};

glm::vec3 SphereLightSource::get_closest_point(glm::vec3 from) {
    return center + glm::normalize(center - from) * radius;
}

FloatColor SphereLightSource::get_effective_color(float r, float cos_theta) {
    return emissive_color * ((strength * cos_theta) / (r * r));
}
