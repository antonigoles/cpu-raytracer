#pragma once
#include "Core/Raytracing/Ray/Ray.hpp"
#include "Core/Raytracing/RayHit/RayHit.hpp"
#include "Core/Scene/Scene.hpp"

class AbstractRayTracingEngine
{
public:
    virtual ~AbstractRayTracingEngine() = default;

    virtual void build_from_scene(std::shared_ptr<Scene> scene);

    virtual RayHit cast_ray(std::shared_ptr<Ray> ray);
};



