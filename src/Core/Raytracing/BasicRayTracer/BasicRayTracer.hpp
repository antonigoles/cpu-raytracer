#pragma once
#include <vector>
#include <Core/FloatColor/FloatColor.hpp>
#include <Core/Scene/Scene.hpp>
#include <Core/Raytracing/Ray/Ray.hpp>
#include <Core/Fragment/Fragment.hpp>
#include <Core/Buffer2D/Buffer2D.hpp>

class BasicRayTracer
{
public:
    BasicRayTracer() {};

    std::vector<FloatColor> gather_light_color(const Scene& scene, const glm::vec3 point, const glm::vec3 normal, const glm::vec3 geometric_normal, float shinines = 1.0f);

    FloatColor cast_ray(const Ray& ray, const Scene& scene, uint32_t depth_left = 8);

    Buffer2D<Fragment> ray_trace_scene(const Scene& scene, uint32_t width, uint32_t height, int sample_per_pixel = 1, float jitter_scale = 0.25f);
};