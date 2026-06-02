#include "Core/Raytracing/BasicRayTracer/PhotonMap.hpp"
#include "Core/Raytracing/Ray/Ray.hpp"
#include <Core/Raytracing/BasicRayTracer/BasicRayTracer.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <cstdlib>
#include <embree4/rtcore.h>
#include <glm/geometric.hpp>
#include <thread>
#include <Misc/Math/Math.hpp>
#include <vector>

static inline float schlick_fresnel(float cosine, float ref_idx) {
    float r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * std::pow(1.0f - cosine, 5.0f);
}

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

FloatColor get_sky_color(glm::vec3 dir_to_sky) 
{
    float t = 0.5f * (dir_to_sky.y + 1.0f); 
    FloatColor horizon = {1.0f, 1.0f, 1.0f, 1.0f};
    FloatColor zenith  = {0.5f, 0.7f, 1.0f, 1.0f};
    FloatColor sky_color = horizon * (1.0f - t) + zenith * t;
    float sky_intensity = 1.0f; 
    return sky_color * sky_intensity;
}

float simple_random()
{
    return (float)std::rand() / (float)RAND_MAX;
}

glm::vec2 simple_2d_random()
{
    return glm::vec2(simple_random(), simple_random());
}

void BasicRayTracer::build_caustic_photon_map(
    SobolSampler& sobol_sampler,
    std::shared_ptr<Scene> scene,
    uint32_t total_photon_count
) {
    std::vector<Photon> photon_vector;

    uint32_t previous_r = 0;
    float total_emissive_area = scene->emissive_triangles.get_total_emissive_area();
    for (ssize_t t_ptr = 0; t_ptr < scene->emissive_triangles.container.size(); t_ptr++) {
        // 0. For each emissive triangle we calculate how many photons it should shoot based on area
        const auto& trig = scene->emissive_triangles.container[t_ptr];
        float area = (trig.r - previous_r);
        float contribution = area / total_emissive_area;
        uint32_t photons_count = total_photon_count * contribution;
        FloatColor emission_per_photon = trig.owner_mesh.material.emission * area * glm::pi<float>() * (1.0f / (float)photons_count);

        for (ssize_t p_cnt = 0; p_cnt < photons_count; p_cnt++) {
            FloatColor photon_emission = emission_per_photon;
            // Shoot photons from this triangle
            
            // 1. Get random point
            auto point = sample_point_on_triangle(trig.trig[0], trig.trig[1], trig.trig[2], simple_2d_random());

            // 2. random hemisphere direction
            auto direction = sample_cosine_hemisphere(trig.normal, simple_2d_random());

            bool had_refraction_bounce = false;
            // 3. Make ray
            auto ray = Ray{.base = point, .direction = direction};
            RayHit ray_hit = rt_engine->intersect(ray);

            // 4. Lets start the ray tracing loop
            while (ray_hit.has_hit) {
                const auto& material = scene->meshes[ray_hit.mesh_index].material;

                if (material.is_emissive) {
                    // I think a photon that hits an emissive triangle should probably not do anything?
                    break;
                }

                float specular_strength = material.specular.strength();
                float diffuse_strength = material.diffuse.strength();
                float transmission_strength = material.transmission.strength();
                float glass_or_mirror_strength = std::max(specular_strength, transmission_strength);
                float total_strength = glass_or_mirror_strength + diffuse_strength;
                if (total_strength <= 0.0f) break; 
                float diffuse_margin = diffuse_strength / total_strength;

                float u = ray_hit.triangle_u;
                float v = ray_hit.triangle_v;
                float w = 1.0f - u - v;

                    
                float choice = sobol_sampler.get_1d();
                
                auto mesh_idx = ray_hit.mesh_index;
                auto triangle_idx = ray_hit.triangle_index;
                const Triangle& triangle = scene->meshes[mesh_idx].triangles[triangle_idx];
                glm::vec3 interpolated_point = ray.base + (ray.direction * ray_hit.distance);
                glm::vec3 n0 = scene->meshes[mesh_idx].normals[triangle.indices[0]];
                glm::vec3 n1 = scene->meshes[mesh_idx].normals[triangle.indices[1]];
                glm::vec3 n2 = scene->meshes[mesh_idx].normals[triangle.indices[2]];
                glm::vec3 interpolated_normal = glm::normalize((w * n0) + (u * n1) + (v * n2));

                if (choice <= diffuse_margin) {
                    // We handle diffuse
                    // if (glm::dot(ray.direction, interpolated_normal) > 0.0f) {
                    //     interpolated_normal = -interpolated_normal;
                    // }

                    if (had_refraction_bounce) {
                        photon_vector.push_back(Photon{
                            .emission = photon_emission,
                            .intersection = interpolated_point,
                            .direction = ray.direction,
                        });
                        break; // one specular bounce should appereantly be enough
                    }
                    break;
                    // float survival_probability = diffuse_strength;
                    // if (sobol_sampler.get_1d() > survival_probability) {
                    //     break;
                    // }
                    // ray = Ray{
                    //     .base = interpolated_point + (interpolated_normal * 0.001f),
                    //     .direction = sample_cosine_hemisphere(interpolated_normal, sobol_sampler.get_2d())
                    // };
                    // photon_emission = photon_emission * material.diffuse * (1.0f / (diffuse_margin * survival_probability));
                } else {
                    if (material.opacity < 1.0f) {
                        float eta_i = 1.0f;
                        float eta_t = material.refraction;
                        glm::vec3 outward_normal = interpolated_normal;
                        float dot_product = glm::dot(ray.direction, interpolated_normal);

                        float refraction_ratio;
                        float cosine;

                        if (dot_product > 0.0f) {
                            // Ray from the inside
                            outward_normal = -interpolated_normal;
                            refraction_ratio = eta_t / eta_i; 
                            cosine = dot_product;
                        } else {
                            // Ray from the outside
                            refraction_ratio = eta_i / eta_t; 
                            cosine = -dot_product;
                        }

                        float reflection_prob = schlick_fresnel(cosine, material.refraction);

                        // Secure against TIR
                        float sin2_theta_t = refraction_ratio * refraction_ratio * (1.0f - cosine * cosine);
                        if (sin2_theta_t > 1.0f) {
                            reflection_prob = 1.0f;
                        }


                        float survival_probability = glass_or_mirror_strength;
                        if (simple_random() > survival_probability) {
                            break;
                        }

                        if (simple_random() < reflection_prob) {
                            ray = Ray{
                                .base = interpolated_point + (outward_normal * 0.001f),
                                .direction = glm::reflect(ray.direction, outward_normal)
                            };

                            photon_emission = (photon_emission * material.specular) * (1.0f / (survival_probability * (1.0f - diffuse_margin)));
                        } else {
                            had_refraction_bounce = true;
                            ray = Ray{
                                .base = interpolated_point - (outward_normal * 0.001f),
                                .direction = glm::refract(ray.direction, outward_normal, refraction_ratio)
                            };

                            photon_emission = (photon_emission * material.transmission) * (1.0f / (survival_probability * (1.0f - diffuse_margin)));                        
                        }
                    } else {
                        float survival_probability = specular_strength;
                        if (simple_random() > survival_probability) break;
                        
                        glm::vec3 hit_normal = interpolated_normal;
                        if (glm::dot(ray.direction, hit_normal) > 0.0f) hit_normal = -hit_normal;

                        ray = Ray{
                            .base = interpolated_point + (hit_normal * 0.001f),
                            .direction = glm::reflect(ray.direction, hit_normal)
                        };
                        photon_emission = (photon_emission * material.specular) * (1.0f / (survival_probability * (1.0f - diffuse_margin)));                    
                    }
                }
                
                ray_hit = rt_engine->intersect(ray);
            }
        }

        previous_r = scene->emissive_triangles.container[t_ptr].r;
    }

    log_info("Caustic map size: ", photon_vector.size());
    this->caustic_photon_map = PhotonMap(photon_vector);
}

FloatColor BasicRayTracer::cast_ray(
    SobolSampler& sobol_sampler, 
    Ray& ray, 
    std::shared_ptr<Scene> scene, 
    uint32_t depth_left, 
    uint32_t nl_parameter
) {
    uint32_t start_depth = depth_left;
    auto radiance = FloatColor{0.0f, 0.0f, 0.0f, 1.0f};
    auto throughput = FloatColor{1.0f, 1.0f, 1.0f, 1.0f};

    float previous_pdf_brdf = 1.0f; 

    std::priority_queue<std::pair<float, uint64_t>> driver_pq;

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
            glm::vec3 face_normal = interpolated_normal;
            if (glm::dot(ray.direction, face_normal) > 0.0f) {
                face_normal = -face_normal;
            }

            if (mesh.material.is_emissive) {
                float cos_light = std::max(0.0f, glm::dot(face_normal, -ray.direction));
                float distance_sq = ray_hit.distance * ray_hit.distance;
                
                if (cos_light > 0.0f) {
                    float mis_weight_brdf = 1.0f;

                    if (nl_parameter > 0 && start_depth - depth_left > 1) {
                        float pdf_light_area = 1.0f / scene->emissive_triangles.get_total_emissive_area();
                        float pdf_light_w = (distance_sq * pdf_light_area) / cos_light;
                        float p_sky = (scene->emissive_triangles.get_total_emissive_area() > 0.0f) ? 0.5f : 1.0f;
                        float adjusted_pdf_nee = ((float)nl_parameter * pdf_light_w) * (1.0f - p_sky);
                        
                        mis_weight_brdf = previous_pdf_brdf / (previous_pdf_brdf + (float)nl_parameter * adjusted_pdf_nee);
                    }
                    
                    radiance = radiance + throughput * (mesh.material.emission) * mis_weight_brdf;
                }
                break;
            }

            ray.near = 0.0001f;
            ray.far = 9999999999999999999999.0f;
            ray.base = interpolated_point + face_normal * this->ray_normal_bias;

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

                if (actual_diffuse.strength() > 0) {
                    actual_diffuse = actual_diffuse * tex_color;
                } else {
                    actual_diffuse = tex_color;
                }
            }

            float diffuse_strength = actual_diffuse.strength();
            float specular_strength = mesh.material.specular.strength();
            float transmission_strength = mesh.material.transmission.strength();
            float glass_or_mirror_strength = std::max(specular_strength, transmission_strength);

            float total_strength = diffuse_strength + glass_or_mirror_strength;
            float diffuse_margin = (total_strength > 0.0f) ? (diffuse_strength / total_strength) : 1.0f;

            FloatColor accumulated_direct_light = {0.0f, 0.0f, 0.0f, 0.0f};

            glm::vec3 ideal_reflection = glm::reflect(ray.direction, face_normal);


            // MIS BLOCK
            float pdf_light_area = 1.0f / scene->emissive_triangles.get_total_emissive_area();

            float p_sky = 0.5f;
            float total_emissive_area = scene->emissive_triangles.get_total_emissive_area();
            if (total_emissive_area <= 0.0f) {
                p_sky = 1.0f;
            }

            for (uint32_t i = 0; i < nl_parameter; i++ ) {
                glm::vec3 dir_to_light;
                float distance = 0.0f;
                float pdf_light_w = 0.0f;
                float cos_wall = 0.0f;
                FloatColor sampled_emission = FloatColor(0.0f, 0.0f, 0.0f, 1.0f);

                float light_choice = sobol_sampler.get_1d();

                if (light_choice < p_sky) {
                    dir_to_light = sample_cosine_hemisphere(face_normal, sobol_sampler.get_2d());
                    cos_wall = glm::dot(face_normal, dir_to_light);
                    
                    if (cos_wall <= 0.0f) continue;
                    
                    distance = 1e8f; 
                    
                    pdf_light_w = (cos_wall / glm::pi<float>()) * p_sky;
                    
                    sampled_emission = get_sky_color(dir_to_light); 

                } else {
                    const auto& emissive_search_result = scene->get_weighted_random_emissivie_triangle(sobol_sampler.get_1d());
                    auto sampled_emissive_point = sample_point_on_triangle(
                        emissive_search_result.trig[0], emissive_search_result.trig[1], emissive_search_result.trig[2],
                        sobol_sampler.get_2d()
                    );

                    glm::vec3 offset = sampled_emissive_point - ray.base;
                    distance = glm::length(offset);
                    dir_to_light = offset / distance;

                    cos_wall = glm::dot(face_normal, dir_to_light);
                    float cos_light = glm::dot(emissive_search_result.normal, -dir_to_light);

                    if (cos_wall <= 0.0f || cos_light < 0.0001f) continue;

                    float distance_sq = distance * distance;
                    
                    pdf_light_w = ((distance_sq * pdf_light_area) / cos_light) * (1.0f - p_sky);
                    
                    sampled_emission = emissive_search_result.owner_mesh.material.emission;
                }

                auto shadow_ray = Ray{
                    .base = ray.base, 
                    .direction = dir_to_light,
                    .near = 0.0001f,
                    .far = distance - 0.0001f
                };
                
                RayHit shadow_ray_hit = rt_engine->occluded(shadow_ray);

                if (!shadow_ray_hit.has_hit) {
                    FloatColor diffuse_brdf = actual_diffuse * (1.0f / glm::pi<float>());
                    float diffuse_pdf = cos_wall / glm::pi<float>();

                    glm::vec3 ideal_reflection = glm::reflect(ray.direction, face_normal);
                    float cos_alpha = glm::dot(ideal_reflection, dir_to_light);
                    
                    FloatColor specular_brdf = FloatColor(0.0f, 0.0f, 0.0f, 1.0f);
                    float specular_pdf = 0.0f;

                    if (cos_alpha > 0.0f) {
                        float ns = mesh.material.shininess;
                        float pow_cos_alpha = glm::pow(cos_alpha, ns);
                        
                        specular_pdf = ((ns + 1.0f) / (2.0f * glm::pi<float>())) * pow_cos_alpha;
                        specular_brdf = mesh.material.specular * (((ns + 2.0f) / (2.0f * glm::pi<float>())) * pow_cos_alpha);
                    }

                    FloatColor total_brdf = diffuse_brdf + specular_brdf;
                    float combined_pdf_brdf = (diffuse_pdf * diffuse_margin) + (specular_pdf * (1.0f - diffuse_margin));
                    
                    float mis_weight_nee = ((float)nl_parameter * pdf_light_w) / ((float)nl_parameter * pdf_light_w + combined_pdf_brdf);
                    
                    FloatColor incoming_light = sampled_emission * total_brdf * cos_wall;
                    accumulated_direct_light = accumulated_direct_light + incoming_light * (mis_weight_nee / pdf_light_w);
                }
            }

            if (nl_parameter > 0) {
                radiance = radiance + throughput * (accumulated_direct_light * (1.0f / (float)nl_parameter));
            }

            if ((start_depth - depth_left) <= 1 && diffuse_margin > 0.1f) {
                uint32_t N_photons = 100; // zhardcodeowane póki co
                std::vector<Photon> found_photons = this->caustic_photon_map.search(interpolated_point, N_photons, 0.5f);
                if (found_photons.size() >= 10) { 
                    float max_dist2 = 0.0f;
                    for (const auto& p : found_photons) {
                        float d2 = glm::distance2(interpolated_point, p.intersection);
                        if (d2 > max_dist2) {
                            max_dist2 = d2;
                        }
                    }

                    float max_dist = std::sqrt(max_dist2);

                    FloatColor total_photon_energy = {0.0f, 0.0f, 0.0f, 0.0f};

                    for (const auto& p : found_photons) {
                        float dist = glm::distance(interpolated_point, p.intersection);
                        
                        float weight = 1.0f - (dist / max_dist); 
                        
                        total_photon_energy = total_photon_energy + p.emission * weight; 
                    }

                    float effective_area = (glm::pi<float>() * max_dist2) / 3.0f;

                    FloatColor diffuse_brdf = actual_diffuse * (1.0f / glm::pi<float>());
                    FloatColor caustic_radiance = diffuse_brdf * total_photon_energy * (1.0f / effective_area);

                    radiance = radiance + throughput * caustic_radiance * diffuse_margin;
                }
            }

            float choice = sobol_sampler.get_1d();

            glm::vec3 selected_direction;
            float current_diffuse_pdf = 0.0f;
            float current_specular_pdf = 0.0f;

            if (choice < diffuse_margin) {
                selected_direction = sample_cosine_hemisphere(face_normal, sobol_sampler.get_2d());
                  
                float cos_scatter = std::max(0.0f, glm::dot(face_normal, selected_direction));
                current_diffuse_pdf = cos_scatter / glm::pi<float>();

                float cos_alpha = glm::dot(ideal_reflection, selected_direction);
                if (cos_alpha > 0.0f) {
                    float ns = mesh.material.shininess;
                    current_specular_pdf = ((ns + 1.0f) / (2.0f * glm::pi<float>())) * glm::pow(cos_alpha, ns);
                }
                
                ray.direction = selected_direction;
                throughput = throughput * actual_diffuse * (1.0f / diffuse_margin);

            } else {
                if (mesh.material.opacity < 1.0f) {
                    // Refraction

                    float eta_i = 1.0f;
                    float eta_t = mesh.material.refraction; 
                    glm::vec3 outward_normal = interpolated_normal;
                    float dot_product = glm::dot(ray.direction, interpolated_normal);

                    float refraction_ratio;
                    float cosine;

                    if (dot_product > 0.0f) {
                        outward_normal = -interpolated_normal;
                        refraction_ratio = eta_t / eta_i; 
                        cosine = dot_product; 
                    } else {
                        refraction_ratio = eta_i / eta_t; 
                        cosine = -dot_product;
                    }

                    float reflection_prob = schlick_fresnel(cosine, mesh.material.refraction);
                    
                    float sin2_theta_t = refraction_ratio * refraction_ratio * (1.0f - cosine * cosine);
                    if (sin2_theta_t > 1.0f) {
                        reflection_prob = 1.0f; 
                    }

                    if (simple_random() < reflection_prob) {
                        ray.base = interpolated_point + outward_normal * 0.001f;
                        ray.direction = glm::reflect(ray.direction, outward_normal);
                        throughput = throughput * mesh.material.specular * (1.0f / ((1.0f - diffuse_margin)));                    
                    } else {
                        ray.base = interpolated_point - outward_normal * 0.001f; 
                        ray.direction = glm::refract(ray.direction, outward_normal, refraction_ratio);
                        throughput = throughput * mesh.material.transmission * (1.0f / ((1.0f - diffuse_margin)));                    
                    }
                    current_specular_pdf = 1.0f; 
                } else {
                    // Refleciton

                    float ns = mesh.material.shininess;

                    glm::vec2 epsilons = sobol_sampler.get_2d();
                    float alpha = 2 * glm::pi<float>() * epsilons.x;
                    float beta = glm::acos( glm::pow(epsilons.y, 1 / (ns + 1)));

                    glm::vec3 helper = (std::abs(ideal_reflection.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
                    glm::vec3 tangent = glm::normalize(glm::cross(helper, ideal_reflection));
                    glm::vec3 bitangent = glm::cross(ideal_reflection, tangent);

                    float sin_beta = glm::sin(beta);
                    float cos_beta = glm::cos(beta);
                    float sin_alpha = glm::sin(alpha);
                    float cos_alpha = glm::cos(alpha);

                    selected_direction = glm::normalize(
                        tangent * (sin_beta * cos_alpha) + 
                        bitangent * (sin_beta * sin_alpha) + 
                        ideal_reflection * cos_beta
                    );

                    float current_specular_pdf = ((ns + 1.0) / (2 * glm::pi<float>())) * glm::pow(glm::cos(beta), ns);

                    float cos_scatter = std::max(0.0f, glm::dot(face_normal, selected_direction));
                    current_diffuse_pdf = cos_scatter / glm::pi<float>();

                    float specular_brdf = ((ns + 2.0f) / (2.0f * glm::pi<float>())) * glm::pow(cos_beta, ns);
                    float specular_weight = specular_brdf / current_specular_pdf;

                    throughput = throughput * mesh.material.specular * specular_weight * (1.0f / (1.0f - diffuse_margin));
                    
                    ray.direction = selected_direction;
                }
            }

            previous_pdf_brdf = (current_diffuse_pdf * diffuse_margin) + (current_specular_pdf * (1.0f - diffuse_margin));
            
            if (start_depth - depth_left > 2) {
                float survival_ppb = std::max(throughput.red, std::max(throughput.green, throughput.blue));
                survival_ppb = std::max(0.1f, survival_ppb);
                if (survival_ppb > simple_random()) {
                    throughput = throughput * (1.0f / survival_ppb);
                } else {
                    break;
                }
            }

            ray.is_coherent = false;
        } else {
            if (scene->emissive_triangles.get_total_emissive_area() == 0.0f) {
                auto sky_color = get_sky_color(ray.direction);
                float mis_weight_brdf = 1.0f;
                if (nl_parameter > 0 && start_depth - depth_left > 1) {
                    float pdf_sky = 1.0f / (4.0f * glm::pi<float>());

                    float p_sky = (scene->emissive_triangles.get_total_emissive_area() > 0.0f) ? 0.5f : 1.0f;
                    float adjusted_pdf_nee = pdf_sky * p_sky;

                    mis_weight_brdf = previous_pdf_brdf / (previous_pdf_brdf + adjusted_pdf_nee);
                }

                radiance = radiance + throughput * sky_color * mis_weight_brdf;
            }
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

    SobolSampler caustic_map_sobol;
    caustic_map_sobol.start_sample(1337, 36, 12);
    log_info("Building caustics map...");
    this->build_caustic_photon_map(caustic_map_sobol, scene, 50000000);
    log_info("Finished building caustics map.");

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