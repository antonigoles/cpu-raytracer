#pragma once
#include <vector>
#include <Core/FloatColor/FloatColor.hpp>
#include <Core/Scene/Scene.hpp>
#include <Core/Raytracing/Ray/Ray.hpp>
#include <Core/Fragment/Fragment.hpp>
#include <Core/Buffer2D/Buffer2D.hpp>
#include <Core/Raytracing/AbstractRayTracingEngine/AbstractRayTracingEngine.hpp>
#include <Misc/Sobol/SobolSampler.hpp>

class BasicRayTracer
{
private:
    std::shared_ptr<AbstractRayTracingEngine> rt_engine;
    float ray_normal_bias;

public:
    BasicRayTracer(std::shared_ptr<AbstractRayTracingEngine> raytracing_engine) {
        this->rt_engine = raytracing_engine;
    };
    
    FloatColor cast_ray(SobolSampler& sobol_sampler, Ray& ray, std::shared_ptr<Scene>, uint32_t depth_left = 8, uint32_t nl_parameter = 1);

    Buffer2D<Fragment> ray_trace_scene(
        std::shared_ptr<Scene> scene, 
        uint32_t width, 
        uint32_t height, 
        uint32_t recursion_depth = 2,
        float ray_normal_bias = 0.0001f,
        int sample_per_pixel = 1, 
        float jitter_scale = 0.25f,
        float focal_length = 1.0f,
        uint32_t nl_parameter = 1
    );

    Buffer2D<FloatColor> ray_trace_scene_hdr(
        std::shared_ptr<Scene> scene, 
        uint32_t width, 
        uint32_t height, 
        uint32_t recursion_depth = 2,
        float ray_normal_bias = 0.0001f,
        int sample_per_pixel = 1, 
        float jitter_scale = 0.25f,
        float focal_length = 1.0f,
        uint32_t nl_parameter = 1
    );
};