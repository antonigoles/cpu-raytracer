#include <Misc/Math/Math.hpp>
#include <random>

template<typename T>
T Math::lerp(T position, T target, float delta_time) {
    return position + (target - position) * delta_time;
};

glm::quat Math::EulerToQuatRadians(float pitch, float yaw, float roll) {
    glm::vec3 eulerAngles(pitch, yaw, roll);
    return glm::quat(eulerAngles);
};

uint32_t Math::fast_random_uint() 
{
    thread_local uint32_t state = []() {
        std::random_device rd;
        uint32_t seed = rd();
        return seed == 0 ? 1 : seed;
    }();

    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    
    return state;
}

float Math::random_float() 
{
    thread_local std::random_device rd;
    thread_local std::mt19937 generator(rd());
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    
    return distribution(generator);
}