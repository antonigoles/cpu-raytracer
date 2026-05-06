#include "AbstractRayTracingEngine.hpp"

#include "Infrastructure/Logger/Logger.hpp"

void AbstractRayTracingEngine::build_from_scene(std::shared_ptr<Scene> scene) {
    log_err("Attempted to call virtual method");
}

RayHit AbstractRayTracingEngine::intersect(Ray& ray) {
    log_err("Attempted to call virtual method");
    return RayHit{};
}

RayHit AbstractRayTracingEngine::occluded(Ray& ray) {
    log_err("Attempted to call virtual method");
    return RayHit{};
}

RayTracerPerformanceMetric AbstractRayTracingEngine::get_performance_metric() {
    log_err("Attempted to call virtual method");
    return RayTracerPerformanceMetric{};
};
