#include <Misc/Math/Math.hpp>

template<typename T>
T Math::lerp(T position, T target, float delta_time) {
    return position + (target - position) * delta_time;
};

glm::quat Math::EulerToQuatRadians(float pitch, float yaw, float roll) {
    glm::vec3 eulerAngles(pitch, yaw, roll);
    return glm::quat(eulerAngles);
};
