#pragma once
#include <embree4/rtcore.h>
#include <vector>
#include <memory>
#include <Core/Scene/Mesh/Mesh.hpp>
#include <Core/Scene/Camera/Camera.hpp>
#include <Core/Scene/Light/PointLightSource/PointLightSource.hpp>
#include <Core/Scene/Light/TriangleLightSource/TriangleLightSource.hpp>
#include <Core/Scene/Light/SphereLightSource/SphereLightSource.hpp>

class WeightedEmissiveTriangleListElement 
{
public:
    float r;
    Mesh& owner_mesh;
    glm::vec3 trig[3];
    glm::vec3 normal;
};

class WeightedEmissiveTriangleList
{
private:
    std::vector<WeightedEmissiveTriangleListElement> container;
public:
    WeightedEmissiveTriangleList();

    void insert(Mesh& owner_mesh, glm::vec3 trig[3]);
    void insert(Mesh& owner_mesh);

    const WeightedEmissiveTriangleListElement& get_random(float random_float);

    float get_total_emissive_area();

    void debug_log_content();
};

class Scene 
{
public:
    WeightedEmissiveTriangleList emissive_triangles;
    std::vector<Mesh> meshes;
    std::shared_ptr<Camera> camera;

    // Legacy
    std::vector<PointLightSource> point_light_sources;
    std::vector<TriangleLightSource> triangle_light_sources;
    std::vector<SphereLightSource> sphere_light_sources;

    const WeightedEmissiveTriangleListElement& get_weighted_random_emissivie_triangle(float random_float);

    ~Scene();
};