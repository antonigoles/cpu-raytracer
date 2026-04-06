#include "Core/Raytracing/HavranKDTreeRayTracingEngine/HavranKDTreeRayTracingEngine.hpp"
#include "Infrastructure/Logger/Logger.hpp"

HavranKDTreeRayTracingEngine::HavranKDTreeRayTracingEngine() = default;

HavranKDTreeRayTracingEngine::~HavranKDTreeRayTracingEngine() {}

void HavranKDTreeRayTracingEngine::build_from_scene(std::shared_ptr<Scene> scene) {
    rays_shot.store(0);
    havran_tree_triangles.clear();
    this->scene = scene;
    
    uint32_t mesh_idx = 0;
    log_info("[HAVRAN_TREE_ENGINE]: Building");
    log_info("[HAVRAN_TREE_ENGINE]: Gathering data...");
    for (const auto & mesh : scene->meshes)
    {
        uint32_t triangle_idx = 0;
        if (mesh.vertices.empty() || mesh.triangles.empty()) continue;
        for (auto& engine_triangle : mesh.triangles) {
            HavranTree::Triangle trig = {
                mesh.vertices[engine_triangle.indices[0]],
                mesh.vertices[engine_triangle.indices[1]],
                mesh.vertices[engine_triangle.indices[2]],
            };
            havran_tree_triangles.push_back(trig);
            tree_trig_idx_to_mesh_triangle_index[havran_tree_triangles.size()-1] = {mesh_idx, triangle_idx};

            triangle_idx++;
        }
        mesh_idx++;
    }
    log_info("[HAVRAN_TREE_ENGINE]: Constructing tree...");
    this->tree = std::make_unique<HavranTree::RSAKDTree>(havran_tree_triangles);
    log_info("[HAVRAN_TREE_ENGINE]: Havran Tree has been built");
};

RayHit HavranKDTreeRayTracingEngine::intersect(std::shared_ptr<Ray> ray) {
    rays_shot++;
    HavranTree::Ray tree_ray = HavranTree::Ray(ray->base + ray->direction * ray->near, ray->direction);
    HavranTree::Hit tree_hit = this->tree->trace(tree_ray);
    return RayHit{
        .ray = ray,
        .has_hit = (ray->far >= tree_hit.t) && (tree_hit.triangle_idx != static_cast<uint32_t>(-1)),
        .distance = tree_hit.t,
        .triangle_index = tree_trig_idx_to_mesh_triangle_index[tree_hit.triangle_idx].second,
        .mesh_index = tree_trig_idx_to_mesh_triangle_index[tree_hit.triangle_idx].first,
        .triangle_u = tree_hit.uv[0],
        .triangle_v = tree_hit.uv[1]
    };
};

RayHit HavranKDTreeRayTracingEngine::occluded(std::shared_ptr<Ray> ray) {
    return this->intersect(ray);
};

RayTracerPerformanceMetric HavranKDTreeRayTracingEngine::get_performance_metric() {
    return RayTracerPerformanceMetric{
        .rays_shot = this->rays_shot.load()
    };
}