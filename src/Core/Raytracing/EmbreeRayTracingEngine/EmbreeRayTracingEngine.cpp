#include "EmbreeRayTracingEngine.hpp"

#include "Infrastructure/Logger/Logger.hpp"

EmbreeRayTracingEngine::EmbreeRayTracingEngine() = default;

EmbreeRayTracingEngine::~EmbreeRayTracingEngine() {
    if (embree_scene) rtcReleaseScene(embree_scene);
    if (embree_device) rtcReleaseDevice(embree_device);
}

RTCRayHit EmbreeRayTracingEngine::get_embree_ray_of_internal_ray(Ray& ray) {
    RTCRayHit query{};

    query.ray.org_x = ray.base.x;
    query.ray.org_y = ray.base.y;
    query.ray.org_z = ray.base.z;

    query.ray.dir_x = ray.direction.x;
    query.ray.dir_y = ray.direction.y;
    query.ray.dir_z = ray.direction.z;

    query.ray.tnear = ray.near;
    query.ray.tfar = ray.far;

    query.ray.mask = -1;
    query.ray.flags = 0;

    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    return query;
}

void handle_embree_error(void* userPtr, enum RTCError error, const char* str) {
    log_err("[Embree Error] ", error, ": ", str);
};

void EmbreeRayTracingEngine::build_from_scene(std::shared_ptr<Scene> scene) {
    rays_shot.store(0);
    this->scene = scene;

    log_info("Building embree");
    embree_device = rtcNewDevice(nullptr);
    if (!embree_device) {
        log_err("Critical: Could not create Embree device!");
        exit(-1);
    }
    rtcSetDeviceErrorFunction(embree_device, handle_embree_error, nullptr);

    embree_scene = rtcNewScene(embree_device);

    for (const auto & mesh : scene->meshes)
    {
        if (mesh.vertices.empty() || mesh.triangles.empty()) continue;

        RTCGeometry geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

        auto* embree_vertices = static_cast<glm::vec3 *>(rtcSetNewGeometryBuffer(
            geom,
            RTC_BUFFER_TYPE_VERTEX,
            0,
            RTC_FORMAT_FLOAT3,
            sizeof(glm::vec3),
            mesh.vertices.size()
        ));


        std::memcpy(embree_vertices, mesh.vertices.data(), mesh.vertices.size() * sizeof(glm::vec3));

        auto* embree_indices = static_cast<Triangle *>(rtcSetNewGeometryBuffer(
            geom,
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT3,
            sizeof(Triangle),
            mesh.triangles.size()
        ));

        std::memcpy(embree_indices, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));

        rtcSetSceneBuildQuality(embree_scene, RTC_BUILD_QUALITY_HIGH);
        rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_HIGH);

        rtcCommitGeometry(geom);
        unsigned int geomID = rtcAttachGeometry(embree_scene, geom);
        rtcReleaseGeometry(geom);
    }

    rtcCommitScene(embree_scene);
    log_info("Embree built successfully");
}

RayHit EmbreeRayTracingEngine::intersect(Ray& ray) {
    // rays_shot++;
    RayHit hit;

    RTCRayHit embree_ray = EmbreeRayTracingEngine::get_embree_ray_of_internal_ray(ray);
    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    if (ray.is_coherent) {
        args.flags = RTC_RAY_QUERY_FLAG_COHERENT;
    } else {
        args.flags = RTC_RAY_QUERY_FLAG_INCOHERENT;
    }
    rtcIntersect1(this->embree_scene, &embree_ray, &args);

    if (embree_ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        hit.has_hit = true;
        hit.distance = embree_ray.ray.tfar;
        hit.triangle_u = embree_ray.hit.u;
        hit.triangle_v = embree_ray.hit.v;
        hit.mesh_index = embree_ray.hit.geomID;
        hit.triangle_index = embree_ray.hit.primID;
    } else {
        hit.has_hit = false;
    }

    hit.ray = ray;

    return hit;
}

RayHit EmbreeRayTracingEngine::occluded(Ray& ray) {
    // rays_shot++;
    RayHit hit;

    RTCRayHit embree_ray = EmbreeRayTracingEngine::get_embree_ray_of_internal_ray(ray);
    RTCOccludedArguments args;
    rtcInitOccludedArguments(&args);
    args.flags = RTC_RAY_QUERY_FLAG_INCOHERENT;
    rtcOccluded1(this->embree_scene, &embree_ray.ray, &args);

    if (embree_ray.ray.tfar < 0.0f) {
        hit.has_hit = true;
        hit.ray.far = embree_ray.ray.tfar;
    } else {
        hit.has_hit = false;
    }

    hit.ray = ray;
    return hit;
}

RayTracerPerformanceMetric EmbreeRayTracingEngine::get_performance_metric() {
    return RayTracerPerformanceMetric{
        .rays_shot = this->rays_shot.load()
    };
}