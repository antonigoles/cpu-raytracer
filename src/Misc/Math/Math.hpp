#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>

class Math
{
public:
    template<typename T>
    static T lerp(T position, T target, float delta_time);

    static glm::quat EulerToQuatRadians(float pitch, float yaw, float roll);

    static uint32_t fast_random_uint();

    static float random_float();

    static glm::vec3 sample_point_on_triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec2& random_sample);

    static glm::vec3 sample_cosine_hemisphere(const glm::vec3& normal, const glm::vec2& random_sample);

    static glm::vec3 sample_blinn_phong_lobe(const glm::vec3& normal, const glm::vec3& incoming_ray_dir, float shininess, const glm::vec2& random_sample);

    static std::tuple<glm::vec3, glm::vec3> create_onb(const glm::vec3& normal);

    static FloatColor lerp_color(const FloatColor& a, const FloatColor& b, float time)
    {
        return a + time * (b - a);
    }

    static FloatColor schlick_color(FloatColor R0, float cos_theta)
    {
        float m = std::pow(1.0f - cos_theta, 5.0f);
        return R0 + (FloatColor(1.0f, 1.0f, 1.0f) - R0) * m;
    }

    static float get_blinn_phong_alpha(float roughness)
    {
        float r = glm::clamp(roughness, 0.001f, 1.0f);
        return (1.0f - r) * (1.0f - r) * 1000.0f;
    }

    static float advanced_blinn_phong_normalization_factor(float alpha)
    {
        // Source: https://www.farbrausch.de/~fg/stuff/phong.pdf
        return (alpha + 2.0f) * (alpha + 4) 
            / (8.0f * glm::pi<float>() * (glm::pow(2, -alpha / 2.0f) + alpha));
    }

    static float basic_blinn_phong_normalization_factor(float alpha) 
    {
        return (alpha + 8.0f) / (8.0f * glm::pi<float>());
    }

    // ALL GGX CODE IS ISOTROPIC

    // Source: https://graphicscompendium.com/theory/08-cook-torrance-ggx
    static float ggx_distribution(glm::vec3 N, glm::vec3 H, float roughness)
    {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = glm::dot(N, H);
        float chi = NdotH > 0 ? 1.0f : 0.0f;
        float NdotH2 = NdotH * NdotH;

        float num = a2;
        float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
        denom = glm::pi<float>() * denom * denom;

        return chi * num / std::max(denom, 0.0000001f);
    }

    // Source: https://ix.cs.uoregon.edu/~hank/441/lectures/pbr_slides.pdf
    static float ggx_geometry_schlick(float n_dot_v, float roughness)
    {
        float r = roughness + 1.0f;
        float k = (r * r) / 8.0f;
        float denom = n_dot_v * (1.0f - k) + k;
        return n_dot_v / denom;
    }

    static float ggx_geometry_smith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness) 
    {
        float NdotV = std::max(glm::dot(N, V), 0.0f);
        float NdotL = std::max(glm::dot(N, L), 0.0f);
        float ggx2 = Math::ggx_geometry_schlick(NdotV, roughness);
        float ggx1 = Math::ggx_geometry_schlick(NdotL, roughness);

        return ggx1 * ggx2;
    }

    static glm::vec3 ggx_sample(glm::vec3 normal, glm::vec3 view_dir, float roughness, glm::vec2 random_sample) {
        // Derived in similar way to blinn-phong
        // Idea: https://computergraphics.stackexchange.com/questions/4979/what-is-importance-sampling
        float a = roughness * roughness;
        float phi = 2.0f * glm::pi<float>() * random_sample.x;
        float cosTheta = glm::sqrt((1.0f - random_sample.y) / (1.0f + (a*a - 1.0f) * random_sample.y));
        float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);

        glm::vec3 H;
        H.x = glm::cos(phi) * sinTheta;
        H.y = glm::sin(phi) * sinTheta;
        H.z = cosTheta;

        glm::vec3 up = std::abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        glm::vec3 world_H = tangent * H.x + bitangent * H.y + normal * H.z;
        world_H = glm::normalize(world_H);

        return glm::reflect(-view_dir, world_H);
    }
};