#pragma once
#include "Core/Raytracing/AbstractRayTracingEngine/AbstractRayTracingEngine.hpp"
#include "Core/Raytracing/HavranKDTreeRayTracingEngine/HavranTree.hpp"
#include "unordered_map"

class HavranKDTreeRayTracingEngine : public AbstractRayTracingEngine
{
private:
    std::vector<HavranTree::Triangle> havran_tree_triangles;
    std::unique_ptr<HavranTree::RSAKDTree> tree;
    std::shared_ptr<Scene> scene;

    std::atomic<uint32_t> rays_shot;

    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> tree_trig_idx_to_mesh_triangle_index;

    static RTCRayHit get_embree_ray_of_internal_ray(std::shared_ptr<Ray> ray);

public:
    HavranKDTreeRayTracingEngine();
    ~HavranKDTreeRayTracingEngine() override;

    void build_from_scene(std::shared_ptr<Scene> scene) override;

    RayHit intersect(std::shared_ptr<Ray> ray) override;

    RayHit occluded(std::shared_ptr<Ray> ray) override;

    RayTracerPerformanceMetric get_performance_metric() override;
};