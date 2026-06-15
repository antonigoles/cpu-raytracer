#include <Misc/Math/Math.hpp>
#include <glm/ext/quaternion_exponential.hpp>
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

glm::vec3 Math::sample_point_on_triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec2& random_sample) 
{
    float sqrt_r1 = std::sqrt(random_sample.x);
    float u = 1.0f - sqrt_r1;
    float v = random_sample.y * sqrt_r1;
    glm::vec3 p = v0 + (v1 - v0) * v + (v2 - v0) * (1.0f - u - v);
    return p;
}

// Metoda Malley'a
glm::vec3 Math::sample_cosine_hemisphere(const glm::vec3& normal, const glm::vec2& random_sample) 
{
    float u1 = random_sample.x;
    float u2 = random_sample.y;

    float r = std::sqrt(u1);
    float theta = 2.0f * glm::pi<float>() * u2;

    float x = r * std::cos(theta);
    float y = r * std::sin(theta);
    float z = std::sqrt(std::max(0.0f, 1.0f - u1));

    float sign = copysignf(1.0f, normal.z);
    const float a = -1.0f / (sign + normal.z);
    const float b = normal.x * normal.y * a;
    
    glm::vec3 tangent(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    glm::vec3 bitangent(b, sign + normal.y * normal.y * a, -normal.y);

    return x * tangent + y * bitangent + z * normal;
}

glm::vec3 Math::sample_blinn_phong_lobe(const glm::vec3& normal, const glm::vec3& incoming_ray_dir, float shininess, const glm::vec2& random_sample)
{
    // Random angle (the one that doesn't care about shininess) 
    float phi = 2.0f * glm::pi<float>() * random_sample.x;

    // getting the other angle - both cos and sin of that angle
    float cos_theta = glm::pow(random_sample.y, 1.0f / (shininess + 1.0f));
    float sin_theta = glm::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));

    // We build "local" H vector (assuming linear space where normal is our "up" vector)
    glm::vec3 H_local(
        sin_theta * glm::cos(phi),
        sin_theta * glm::sin(phi),
        cos_theta
    );

    // We create ortonormal basis of the linear space described above
    auto [tangent, bitangent] = Math::create_onb(normal);
    
    // Now we use this basis to move our H_local vector to world coordinates
    glm::vec3 H_world = glm::normalize(tangent * H_local.x + bitangent * H_local.y + normal * H_local.z);
    return glm::reflect(incoming_ray_dir, H_world);
}

std::tuple<glm::vec3, glm::vec3> Math::create_onb(const glm::vec3& normal)
{
    glm::vec3 out_tangent; 
    glm::vec3 out_bitangent;

    // Stable way of finding other 2 vectors for a basis created with normal as the "up" vector
    if (std::abs(normal.z) > 0.99999f) {
        out_tangent = glm::vec3(0.0f, -1.0f, 0.0f);
        out_bitangent = glm::vec3(-1.0f, 0.0f, 0.0f);
    } else {
        out_tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), normal));
        out_bitangent = glm::normalize(glm::cross(normal, out_tangent));
    }

    return {out_tangent, out_bitangent};
}