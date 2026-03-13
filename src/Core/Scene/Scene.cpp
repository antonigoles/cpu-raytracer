#include <Core/Scene/Scene.hpp>
#include <Infrastructure/Logger/Logger.hpp>

Scene::~Scene() 
{
    if (embree_scene) rtcReleaseScene(embree_scene);
    if (embree_device) rtcReleaseDevice(embree_device);
}

void handle_embree_error(void* userPtr, enum RTCError error, const char* str) {
    log_err("[Embree Error] ", error, ": ", str);
};

void Scene::build_embree()
{
    log_info("Building embree");
    embree_device = rtcNewDevice(nullptr);
    if (!embree_device) {
        log_err("Critical: Could not create Embree device!");
        exit(-1);
    }
    rtcSetDeviceErrorFunction(embree_device, handle_embree_error, nullptr);

    embree_scene = rtcNewScene(embree_device);

    for (size_t mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) 
    {
        const Mesh& mesh = meshes[mesh_idx];
        
        if (mesh.vertices.empty() || mesh.triangles.empty()) continue;

        RTCGeometry geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

        glm::vec3* embree_vertices = (glm::vec3*)rtcSetNewGeometryBuffer(
            geom, 
            RTC_BUFFER_TYPE_VERTEX, 
            0, 
            RTC_FORMAT_FLOAT3, 
            sizeof(glm::vec3), 
            mesh.vertices.size()
        );
        

        std::memcpy(embree_vertices, mesh.vertices.data(), mesh.vertices.size() * sizeof(glm::vec3));

        Triangle* embree_indices = (Triangle*)rtcSetNewGeometryBuffer(
            geom, 
            RTC_BUFFER_TYPE_INDEX, 
            0, 
            RTC_FORMAT_UINT3, 
            sizeof(Triangle), 
            mesh.triangles.size()
        );

        std::memcpy(embree_indices, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));

        rtcCommitGeometry(geom);
        
        unsigned int geomID = rtcAttachGeometry(embree_scene, geom);
        
        rtcReleaseGeometry(geom);
    }

    rtcCommitScene(embree_scene);
    log_info("Embree built successfully");
}