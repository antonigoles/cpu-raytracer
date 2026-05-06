#pragma once
#include "Core/Raytracing/AbstractRayTracingEngine/AbstractRayTracingEngine.hpp"

class EmbreeRayTracingEngine : public AbstractRayTracingEngine
{
private:
    RTCDevice embree_device = nullptr;
    RTCScene embree_scene = nullptr;
    std::shared_ptr<Scene> scene;

    std::atomic<uint32_t> rays_shot;

    static RTCRayHit get_embree_ray_of_internal_ray(Ray& ray);

public:
    EmbreeRayTracingEngine();
    ~EmbreeRayTracingEngine() override;

    void build_from_scene(std::shared_ptr<Scene> scene) override;

    RayHit intersect(Ray& ray) override;

    RayHit occluded(Ray& ray) override;

    RayTracerPerformanceMetric get_performance_metric() override;
};
