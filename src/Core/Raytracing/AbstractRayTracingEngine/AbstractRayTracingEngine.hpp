#pragma once
#include "Core/Raytracing/Ray/Ray.hpp"
#include "Core/Raytracing/RayHit/RayHit.hpp"
#include "Core/Scene/Scene.hpp"
#include <atomic>

class RayTracerPerformanceMetric {
public:
    uint32_t rays_shot;
};

class AbstractRayTracingEngine
{
public:
    virtual ~AbstractRayTracingEngine() = default;

    virtual void build_from_scene(std::shared_ptr<Scene> scene);

    virtual RayHit occluded(std::shared_ptr<Ray> ray);

    virtual RayHit intersect(std::shared_ptr<Ray> ray);

    virtual RayTracerPerformanceMetric get_performance_metric();
};



