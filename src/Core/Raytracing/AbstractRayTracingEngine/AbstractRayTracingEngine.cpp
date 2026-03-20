#include "AbstractRayTracingEngine.hpp"

#include "Infrastructure/Logger/Logger.hpp"

void AbstractRayTracingEngine::build_from_scene(std::shared_ptr<Scene> scene) {
    log_err("Attempted to call virtual method");
}

RayHit AbstractRayTracingEngine::cast_ray(std::shared_ptr<Ray> ray) {
    log_err("Attempted to call virtual method");
    return RayHit{};
}
