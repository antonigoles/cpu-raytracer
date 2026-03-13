#include <Core/Scene/Light/PointLightSource/PointLightSource.hpp>

FloatColor PointLightSource::get_effective_color(float r, float cos_theta) {
    return color * ((strength * cos_theta) / (4.0f * 3.14159f * r * r));
}
