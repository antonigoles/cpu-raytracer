#pragma once
#include <glm/gtx/quaternion.hpp>

class Math
{
public:
    template<typename T>
    static T lerp(T position, T target, float delta_time);

    static glm::quat EulerToQuatRadians(float pitch, float yaw, float roll);

    static uint32_t fast_random_uint();

    static float random_float();
};