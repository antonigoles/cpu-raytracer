#include <Core/Raytracing/BasicRayTracer/BasicRayTracer.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <embree4/rtcore.h>
#include <thread>
#include <random>
#include <Misc/Math/Math.hpp>

// Metoda Malley'a
glm::vec3 sample_cosine_hemisphere(const glm::vec3& normal, const glm::vec2& random_sample) 
{
    float u1 = random_sample.x;
    float u2 = random_sample.y;

    float r = std::sqrt(u1);
    float theta = 2.0f * glm::pi<float>() * u2;

    float x = r * std::cos(theta);
    float y = r * std::sin(theta);
    float z = std::sqrt(std::max(0.0f, 1.0f - u1));

    float sign = copysignf(1.0f, normal.z);
    const float a = -1.0f / (sign + normal.z);
    const float b = normal.x * normal.y * a;
    
    glm::vec3 tangent(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
    glm::vec3 bitangent(b, sign + normal.y * normal.y * a, -normal.y);

    return x * tangent + y * bitangent + z * normal;
}

glm::vec3 sample_point_on_triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec2& random_sample) 
{
    float sqrt_r1 = std::sqrt(random_sample.x);
    float u = 1.0f - sqrt_r1;
    float v = random_sample.y * sqrt_r1;
    glm::vec3 p = v0 + (v1 - v0) * v + (v2 - v0) * (1.0f - u - v);
    return p;
}

FloatColor BasicRayTracer::cast_ray(SobolSampler& sobol_sampler, Ray& ray, std::shared_ptr<Scene> scene, uint32_t depth_left, uint32_t nl_parameter) {
    uint32_t start_depth = depth_left;
    auto radiance = FloatColor{0.0f, 0.0f, 0.0f, 1.0f};
    auto throughput = FloatColor{1.0f, 1.0f, 1.0f, 1.0f};

    float previous_pdf_brdf = 1.0f; 
    bool last_bounce_specular = true;

    while (depth_left --> 0) {
        RayHit ray_hit = rt_engine->intersect(ray);
        if (ray_hit.has_hit)
        {            
            uint32_t mesh_idx = ray_hit.mesh_index;
            uint32_t triangle_idx = ray_hit.triangle_index;
            const Triangle& triangle = scene->meshes[mesh_idx].triangles[triangle_idx];
            const Mesh& mesh = scene->meshes[mesh_idx];

            float u = ray_hit.triangle_u;
            float v = ray_hit.triangle_v;
            float w = 1.0f - u - v;

            glm::vec3 interpolated_point = ray.base + (ray.direction * ray_hit.distance);

            glm::vec3 n0 = scene->meshes[mesh_idx].normals[triangle.indices[0]];
            glm::vec3 n1 = scene->meshes[mesh_idx].normals[triangle.indices[1]];
            glm::vec3 n2 = scene->meshes[mesh_idx].normals[triangle.indices[2]];
            glm::vec3 interpolated_normal = glm::normalize((w * n0) + (u * n1) + (v * n2));
            if (glm::dot(ray.direction, interpolated_normal) > 0.0f) {
                interpolated_normal = -interpolated_normal;
            }

            if (mesh.material.is_emissive) {
                if (last_bounce_specular) {
                    radiance = radiance + throughput * mesh.material.emission;
                } else {
                    float cos_light = std::max(0.0f, glm::dot(interpolated_normal, -ray.direction));
                    float distance_sq = ray_hit.distance * ray_hit.distance;
                    
                    if (cos_light > 0.0f) {
                        float mis_weight_brdf = 1.0f;

                        if (nl_parameter > 0) {
                            float pdf_light_area = 1.0f / scene->emissive_triangles.get_total_emissive_area();
                            float pdf_light_w = (distance_sq * pdf_light_area) / cos_light;
                            
                            mis_weight_brdf = previous_pdf_brdf / (previous_pdf_brdf + (float)nl_parameter * pdf_light_w);
                        }
                        
                        radiance = radiance + throughput * mesh.material.emission * mis_weight_brdf;
                    }
                }
                break;
            }

            ray.near = 0.0001f;
            ray.far = std::numeric_limits<float>::infinity();
            ray.base = interpolated_point + interpolated_normal * this->ray_normal_bias;

            FloatColor actual_diffuse = mesh.material.diffuse;

            if (!mesh.texture.is_empty)
            {
                glm::vec2 txt_uv_0 = scene->meshes[mesh_idx].texture_coords[triangle.indices[0]];
                glm::vec2 txt_uv_1 = scene->meshes[mesh_idx].texture_coords[triangle.indices[1]];
                glm::vec2 txt_uv_2 = scene->meshes[mesh_idx].texture_coords[triangle.indices[2]];

                glm::vec2 texture_uv = (w * txt_uv_0) + (u * txt_uv_1) + (v * txt_uv_2);

                FloatColor tex_color = mesh.texture.sample(texture_uv.x, texture_uv.y, ray_hit.distance).as_floats();

                tex_color.red   = std::pow(tex_color.red,   2.2f);
                tex_color.green = std::pow(tex_color.green, 2.2f);
                tex_color.blue  = std::pow(tex_color.blue,  2.2f);

                actual_diffuse = actual_diffuse * tex_color;
            }

            float diffuse_strength = actual_diffuse.strength();
            float specular_strength = mesh.material.specular.strength();
            float total_strength = diffuse_strength + specular_strength;
            float diffuse_margin = (total_strength > 0.0f) ? (diffuse_strength / total_strength) : 1.0f;

            FloatColor accumulated_direct_light = {0.0f, 0.0f, 0.0f, 0.0f};

            if (diffuse_margin > 0.0f) {
                float pdf_light_area = 1.0f / scene->emissive_triangles.get_total_emissive_area();
                FloatColor brdf = actual_diffuse * (1.0f / glm::pi<float>());

                for (uint32_t i = 0; i < nl_parameter; i++ ) {
                    const WeightedEmissiveTriangleListElement& emissive_search_result = scene->get_weighted_random_emissivie_triangle(sobol_sampler.get_1d());
                    auto sampled_emissive_point = sample_point_on_triangle(
                        emissive_search_result.trig[0], emissive_search_result.trig[1], emissive_search_result.trig[2],
                        sobol_sampler.get_2d()
                    );

                    glm::vec3 offset = sampled_emissive_point - ray.base;
                    float distance = glm::length(offset);
                    glm::vec3 dir_to_light = offset / distance;

                    float cos_wall = glm::dot(interpolated_normal, dir_to_light);

                    glm::vec3 light_normal = emissive_search_result.normal;
                    // if (glm::dot(dir_to_light, light_normal) > 0.0f) {
                    //     light_normal = -light_normal;
                    // }

                    float cos_light = glm::dot(light_normal, -dir_to_light);

                    if (cos_wall <= 0.0f || cos_light < 0.0001f) {
                        continue; 
                    }

                    auto shadow_ray = Ray{
                        .base = ray.base, 
                        .direction = dir_to_light,
                        .near = 0.0001f,
                        .far = distance - 0.00001f
                    };
                    
                    RayHit shadow_ray_hit = rt_engine->occluded(shadow_ray);

                    if (!shadow_ray_hit.has_hit) {
                        float distance_sq = distance * distance;
                        
                        float pdf_light_w = (distance_sq * pdf_light_area) / cos_light;
                        float pdf_brdf_w = (cos_wall / glm::pi<float>()) * diffuse_margin;
                        float mis_weight_nee = ((float)nl_parameter * pdf_light_w) / ((float)nl_parameter * pdf_light_w + pdf_brdf_w);

                        FloatColor incoming_light = emissive_search_result.owner_mesh.material.emission * brdf * cos_wall;
                        accumulated_direct_light = accumulated_direct_light + incoming_light * (mis_weight_nee / pdf_light_w);
                    }
                }

                if (nl_parameter > 0) {
                    radiance = radiance + throughput * (accumulated_direct_light * (1.0f / (float)nl_parameter));
                }
            }

            float choice = sobol_sampler.get_1d();

            if (choice < diffuse_margin) {
                ray.direction = sample_cosine_hemisphere(interpolated_normal, sobol_sampler.get_2d());
                
                throughput = throughput * actual_diffuse * (1.0f / diffuse_margin);
                
                float cos_scatter = std::max(0.0f, glm::dot(interpolated_normal, ray.direction));
                previous_pdf_brdf = (cos_scatter / glm::pi<float>()) * diffuse_margin;
                
                last_bounce_specular = false;

            } else {
                ray.direction = glm::reflect(ray.direction, interpolated_normal);
                
                throughput = throughput * mesh.material.specular * (1.0f / (1.0f - diffuse_margin));
                
                last_bounce_specular = true;
            }
            
            if (start_depth - depth_left > 2) {
                float survival_ppb = std::max(throughput.red, std::max(throughput.green, throughput.blue));
                survival_ppb = std::max(0.1f, survival_ppb);
                if (survival_ppb > sobol_sampler.get_1d()) {
                    throughput = throughput * (1.0f / survival_ppb);
                } else {
                    break;
                }
            }

            ray.is_coherent = false;
        } else {
            float t = 0.5f * (ray.direction.y + 1.0f); 
            FloatColor horizon = {1.0f, 1.0f, 1.0f, 1.0f};
            FloatColor zenith  = {0.5f, 0.7f, 1.0f, 1.0f};
            FloatColor sky_color = horizon * (1.0f - t) + zenith * t;
            float sky_intensity = 1.0f; 
            radiance = radiance + throughput * (sky_color * sky_intensity);
            break;
        }
    }

    return radiance;
}

Buffer2D<Fragment> BasicRayTracer::ray_trace_scene(
    std::shared_ptr<Scene> scene, 
    uint32_t width, 
    uint32_t height, 
    uint32_t recursion_depth,
    float ray_normal_bias,
    int sample_per_pixel, 
    float jitter_scale,
    float focal_length,
    uint32_t nl_parameter
) {
    auto result = this->ray_trace_scene_hdr(
        scene, 
        width, 
        height, 
        recursion_depth, 
        ray_normal_bias, 
        sample_per_pixel,
        jitter_scale,
        focal_length,
        nl_parameter
    );

    FloatColor c;
    Buffer2D<Fragment> buffer(width, height);
    for (int x = 0; x<width; x++) {
        for (int y = 0; y<width; y++) {
            c.red = result.at(x, y)->red;
            c.green = result.at(x, y)->green;
            c.blue = result.at(x, y)->blue;
            c.alpha = result.at(x, y)->alpha;
            buffer.write(x, y, Fragment(
                Color::rasterize_from_float_color(c) 
            ));
        }
    }

    return buffer;
};

Buffer2D<FloatColor> BasicRayTracer::ray_trace_scene_hdr(
    std::shared_ptr<Scene> scene, 
    uint32_t width, 
    uint32_t height, 
    uint32_t recursion_depth,
    float ray_normal_bias,
    int sample_per_pixel, 
    float jitter_scale,
    float focal_length,
    uint32_t nl_parameter
) {   
    Buffer2D<FloatColor> buffer(width, height);

    this->ray_normal_bias = ray_normal_bias;

    float hfov_tan = glm::tan(scene->camera->fov / 2.0f);
    float aspect_ratio = ((float)width) / ((float)height);
    
    glm::vec3 cam_pos = scene->camera->position;
    glm::vec3 cam_right = scene->camera->get_right();
    glm::vec3 cam_up = scene->camera->get_up();
    glm::vec3 cam_forward = scene->camera->get_forward() * focal_length;

    std::atomic<uint32_t> current_y{0};

    glm::vec2 sample_center = glm::vec2(
        glm::floor((float)sample_per_pixel / 2.0f), 
        glm::floor((float)sample_per_pixel / 2.0f)
    );

    float pixel_physical_width = (hfov_tan * aspect_ratio * 2.0f) / (float)width;
    float pixel_physical_height = (hfov_tan * 2.0f) / (float)height;

    auto worker_thread = [&]() 
    {
        SobolSampler sampler;
        uint32_t y;
        
        while ((y = current_y.fetch_add(1)) < height) 
        {
            float p_y = hfov_tan * (1.0f - ((float)y + 0.5f) / ((float)height) * 2.0f);

            for (uint32_t x = 0; x < width; x++) 
            {
                float p_x = hfov_tan * aspect_ratio * (((float)x + 0.5f) / ((float)width) * 2.0f - 1.0f);

                FloatColor sum_color = Color(0, 0, 0, 0).as_floats();
                
                for (int sy = 0; sy < sample_per_pixel; sy++) {
                    for (int sx = 0; sx < sample_per_pixel; sx++) {

                        sampler.start_sample(sy * sample_per_pixel + sx, x, y);
                        
                        glm::vec2 sample_point = glm::vec2((float)sy, (float)sx);
                        
                        glm::vec2 sample_jitter = (sample_point - sample_center) * jitter_scale;
                        
                        float p_y_prim = p_y + (sample_jitter.y * pixel_physical_height);
                        float p_x_prim = p_x + (sample_jitter.x * pixel_physical_width);

                        Ray ray;
                        ray.is_coherent = true;
                        ray.base = cam_pos;
                        ray.direction = glm::normalize(p_x_prim * cam_right + p_y_prim * cam_up + cam_forward);
                        sum_color = sum_color + this->cast_ray(sampler, ray, scene, recursion_depth, nl_parameter);
                    }
                }

                sum_color = sum_color * (1.0f / (sample_per_pixel * sample_per_pixel));
                
                buffer.write(x, y, sum_color);
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