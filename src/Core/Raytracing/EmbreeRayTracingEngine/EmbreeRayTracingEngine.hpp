#pragma once
#include "Core/Raytracing/AbstractRayTracingEngine/AbstractRayTracingEngine.hpp"

class EmbreeRayTracingEngine : public AbstractRayTracingEngine
{
private:
    RTCDevice embree_device = nullptr;
    RTCScene embree_scene = nullptr;
    std::shared_ptr<Scene> scene;

    static RTCRayHit get_embree_ray_of_internal_ray(std::shared_ptr<Ray> ray);

public:
    EmbreeRayTracingEngine();
    ~EmbreeRayTracingEngine() override;

    void build_from_scene(std::shared_ptr<Scene> scene) override;

    RayHit cast_ray(std::shared_ptr<Ray> ray) override;
};
