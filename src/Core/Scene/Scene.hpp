#pragma once
#include <embree4/rtcore.h>
#include <vector>
#include <memory>
#include <Core/Scene/Mesh/Mesh.hpp>
#include <Core/Scene/Camera/Camera.hpp>
#include <Core/Scene/Light/PointLightSource/PointLightSource.hpp>
#include <Core/Scene/Light/TriangleLightSource/TriangleLightSource.hpp>
#include <Core/Scene/Light/SphereLightSource/SphereLightSource.hpp>


class Scene 
{
public:
    std::vector<Mesh> meshes;
    std::shared_ptr<Camera> camera;
    std::vector<PointLightSource> point_light_sources;
    std::vector<TriangleLightSource> triangle_light_sources;
    std::vector<SphereLightSource> sphere_light_sources;

    RTCDevice embree_device = nullptr;
    RTCScene embree_scene = nullptr;

    ~Scene();

    void build_embree();
};