#include <Core/Raytracing/BasicRayTracer/BasicRayTracer.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <embree4/rtcore.h>
#include <thread>

std::vector<FloatColor> BasicRayTracer::gather_light_color(std::shared_ptr<Scene> scene, const glm::vec3 point, const glm::vec3 normal, const glm::vec3 geometric_normal, float shinines) {
    auto diffuse = FloatColor(0,0,0,0);
    auto specular = FloatColor(0,0,0,0);

    for (auto point_light_source : scene->point_light_sources) {
        auto norm_direction_to_light = glm::normalize(point_light_source.position - point);
        glm::vec3 safe_geometric_normal = geometric_normal;

        glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

        auto distance_to_light = glm::distance(point, point_light_source.position);
        std::shared_ptr<Ray> light_ray = std::make_shared<Ray>(
            safe_shadow_origin,
            norm_direction_to_light,
            0.00001,
            distance_to_light - 0.001f
        );
        
        auto ray_hit = rt_engine->cast_ray(light_ray);

        if (ray_hit.has_hit) continue;

        diffuse = diffuse + point_light_source.get_effective_color(
            distance_to_light,
            glm::max(0.0f, glm::dot(normal, norm_direction_to_light))
        );
    }


    for (auto triangle_light_source : scene->triangle_light_sources) {
        // randomly sampled point on triangle
        auto light_point = triangle_light_source.sample_random_point();

        auto norm_direction_to_light = glm::normalize(light_point - point);
        glm::vec3 safe_geometric_normal = geometric_normal;

        glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

        auto distance_to_light = glm::distance(point, light_point);
        std::shared_ptr<Ray> light_ray = std::make_shared<Ray>(
            safe_shadow_origin,
            norm_direction_to_light,
            0.00001,
            distance_to_light - 0.001f
        );
        
        auto ray_hit = rt_engine->cast_ray(light_ray);

        if (ray_hit.has_hit) continue;

        diffuse = diffuse + triangle_light_source.get_effective_color(
            distance_to_light,
            std::max(0.0f, glm::dot(normal, norm_direction_to_light)),
            std::max(0.0f, glm::dot(triangle_light_source.normal, -norm_direction_to_light))
        );

        glm::vec3 Lj = light_point - point;
        glm::vec3 V = scene->camera->position - point;
        glm::vec3 Hj = glm::normalize(Lj + V);

        specular = specular + triangle_light_source.get_effective_color(
            distance_to_light,
            glm::pow(std::max(0.0f, glm::dot(normal, Hj)), shinines),
            std::max(0.0f, glm::dot(triangle_light_source.normal, -norm_direction_to_light))
        );
    }

    for (auto sphere_light_source : scene->sphere_light_sources) {
        // randomly sampled point on triangle
        auto light_point = sphere_light_source.get_closest_point(point);

        auto norm_direction_to_light = glm::normalize(light_point - point);
        glm::vec3 safe_geometric_normal = geometric_normal;

        glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

        auto distance_to_light = glm::distance(point, light_point);
        std::shared_ptr<Ray> light_ray = std::make_shared<Ray>(
            safe_shadow_origin,
            norm_direction_to_light,
            0.00001,
            distance_to_light - 0.001f
        );
        
        auto ray_hit = rt_engine->cast_ray(light_ray);

        if (ray_hit.has_hit) continue;

        diffuse = diffuse + sphere_light_source.get_effective_color(
            distance_to_light,
            std::max(0.0f, glm::dot(normal, norm_direction_to_light))
        );

        glm::vec3 Lj = light_point - point;
        glm::vec3 V = scene->camera->position - point;
        glm::vec3 Hj = glm::normalize(Lj + V);

        specular = specular + sphere_light_source.get_effective_color(
            distance_to_light,
            glm::pow(std::max(0.0f, glm::dot(normal, Hj)), shinines)
        );
    }

    return {diffuse, specular};
}

FloatColor BasicRayTracer::cast_ray(std::shared_ptr<Ray> ray, std::shared_ptr<Scene> scene, uint32_t depth_left) {
    auto color = FloatColor{0, 0, 0, 255};
    if (depth_left == 0) {
        return color;
    }

    RayHit ray_hit = rt_engine->cast_ray(ray);

    auto ambient_light = Color(20, 30, 50, 255).as_floats();

    if (ray_hit.has_hit)
    {
        uint32_t mesh_idx = ray_hit.mesh_index;
        uint32_t triangle_idx = ray_hit.triangle_index;
        const Triangle& triangle = scene->meshes[mesh_idx].triangles[triangle_idx];
        const Mesh& mesh = scene->meshes[mesh_idx];

        float u = ray_hit.triangle_u;
        float v = ray_hit.triangle_v;
        float w = 1.0f - u - v;

        glm::vec3 interpolated_point = ray->base + (ray->direction * ray_hit.distance);

        glm::vec3 n0 = scene->meshes[mesh_idx].normals[triangle.indices[0]];
        glm::vec3 n1 = scene->meshes[mesh_idx].normals[triangle.indices[1]];
        glm::vec3 n2 = scene->meshes[mesh_idx].normals[triangle.indices[2]];

        glm::vec3 p0 = scene->meshes[mesh_idx].vertices[triangle.indices[0]];
        glm::vec3 p1 = scene->meshes[mesh_idx].vertices[triangle.indices[1]];
        glm::vec3 p2 = scene->meshes[mesh_idx].vertices[triangle.indices[2]];

        glm::vec3 geometric_normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        glm::vec3 interpolated_normal = (w * n0) + (u * n1) + (v * n2);
        // interpolated_normal = glm::normalize( interpolated_normal);

        if (glm::dot(geometric_normal, interpolated_normal) < 0) {
            geometric_normal = -geometric_normal;
        }

        auto next_ray = std::make_shared<Ray>(
            interpolated_point,
            glm::reflect(ray->direction, interpolated_normal) 
        );
        
        auto diffuse_reflectance = mesh.material.diffuse;

        if (!mesh.texture.is_empty)
        {
            glm::vec2 txt_uv_0 = scene->meshes[mesh_idx].texture_coords[triangle.indices[0]];
            glm::vec2 txt_uv_1 = scene->meshes[mesh_idx].texture_coords[triangle.indices[1]];
            glm::vec2 txt_uv_2 = scene->meshes[mesh_idx].texture_coords[triangle.indices[2]];

            glm::vec2 texture_uv = (w * txt_uv_0) + (u * txt_uv_1) + (v * txt_uv_2);

            diffuse_reflectance = mesh.texture.sample(texture_uv.x, texture_uv.y, ray_hit.distance).as_floats();
        }

        if (mesh.material.illumination == 0) {
            return diffuse_reflectance;
        }

        std::vector<FloatColor> light_color = gather_light_color(scene, interpolated_point, interpolated_normal, geometric_normal, mesh.material.shininess);


        FloatColor recast_result = cast_ray(next_ray, scene, depth_left - 1);

        ambient_light = diffuse_reflectance * 0.1f;

        return mesh.material.ambient * ambient_light 
            + diffuse_reflectance * light_color[0]
            + mesh.material.specular * (light_color[1] + recast_result);
    }

    return color;
};

Buffer2D<Fragment> BasicRayTracer::ray_trace_scene(std::shared_ptr<Scene> scene, uint32_t width, uint32_t height, int sample_per_pixel, float jitter_scale)
{   
    Buffer2D<Fragment> buffer(width, height);

    float hfov_tan = glm::tan(scene->camera->fov / 2.0f);
    float aspect_ratio = ((float)width) / ((float)height);
    
    glm::vec3 cam_pos = scene->camera->position;
    glm::vec3 cam_right = scene->camera->right();
    glm::vec3 cam_up = scene->camera->up();
    glm::vec3 cam_forward = scene->camera->forward();

    std::atomic<uint32_t> current_y{0};

    glm::vec2 sample_center = glm::vec2((float)sample_per_pixel / 2.0f, (float)sample_per_pixel / 2.0f);

    auto worker_thread = [&]() 
    {
        uint32_t y;
        
        while ((y = current_y.fetch_add(1)) < height) 
        {
            float p_y = hfov_tan * (1.0f - ((float)y + 0.5f) / ((float)height) * 2.0f);

            for (uint32_t x = 0; x < width; x++) 
            {
                // TODO: Maybe make weighted sampling
                FloatColor sum_color = Color(0, 0, 0, 0).as_floats();
                for (int sy = 0; sy < sample_per_pixel; sy++) {
                    for (int sx = 0; sx < sample_per_pixel; sx++) {
                        glm::vec2 sample_point = glm::vec2((float)sy, (float)sx);
                        glm::vec2 sample_jitter = (sample_point - sample_center) * jitter_scale;

                        float p_x = hfov_tan * aspect_ratio * (((float)x + 0.5f) / ((float)width) * 2.0f - 1.0f);

                        float p_y_prim = p_y + sample_jitter.y;
                        float p_x_prim = p_x + sample_jitter.x;

                        std::shared_ptr<Ray> ray = std::make_shared<Ray>(
                            cam_pos,
                            glm::normalize(p_x_prim * cam_right + p_y_prim * cam_up + cam_forward)
                        );

                        sum_color = sum_color + this->cast_ray(ray, scene);
                    }
                }

                sum_color = sum_color * (1.0f / (sample_per_pixel * sample_per_pixel));
                
                buffer.write(x, y, Fragment(
                    Color::rasterize_from_float_color(sum_color) 
                ));
            }
        }
    };

    uint32_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (uint32_t i = 0; i < num_threads; i++) {
        threads.emplace_back(worker_thread);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return buffer;
}