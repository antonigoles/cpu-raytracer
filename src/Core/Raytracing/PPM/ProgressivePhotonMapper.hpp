#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include "Core/Raytracing/AbstractRayTracingEngine/AbstractRayTracingEngine.hpp"
#include "Core/Raytracing/BasicRayTracer/PhotonMap.hpp"
#include "Core/Raytracing/Ray/Ray.hpp"
#include "Core/Raytracing/RayHit/RayHit.hpp"
#include "Core/Scene/Material/PBRMaterial.hpp"
#include "Infrastructure/Logger/Logger.hpp"
#include "Misc/Math/Math.hpp"
#include <Core/Scene/Scene.hpp>
#include <Core/Buffer2D/Buffer2D.hpp>
#include <cstdlib>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <mutex>
#include <queue>
#include <random>
#include <sys/types.h>
#include <vector>


class ProgressivePhotonMapper
{
private:
    std::shared_ptr<AbstractRayTracingEngine> rt_engine;
    std::shared_ptr<Scene> scene;
    Buffer2D<FloatColor> result;
    Buffer2D<FloatColor> direct_emission_buffer;
    float alpha;
    ssize_t photons_per_pass;

    std::mutex tile_queue_mutex;
    std::mutex hitpoint_buffer_mutex;
    std::mutex photon_queue_mutex;
    std::mutex photon_vector_mutex;
    std::mutex pixel_queue_mutex;

    PhotonMap photon_map;

    float radius;

    ssize_t photon_gather_limit;

    float total_photons_emitted_so_far = 0;

    class HitPoint {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 direction;
        const PBRMaterial *material = nullptr;
        uint32_t pixel_x;
        uint32_t pixel_y;
        FloatColor pixel_weight = FloatColor(1.0f, 1.0f, 1.0f);
        float radius_squared;
        float photon_count = 0.0f;
        FloatColor accumulated_flux = FloatColor(0.0f, 0.0f, 0.0f);
    };

    class PhotonRay{
    public:
        FloatColor photon_emission;
        Ray ray;
    };

    std::vector<HitPoint> hitpoint_buffer;

    class HitData {
    public:
        const Triangle triangle;
        const Mesh* mesh;
        const glm::vec3 face_normal;
        const glm::vec3 interpolated_normal;
        const glm::vec3 interpolated_point;
    };

    HitData resolve_hit_data(const RayHit& ray_hit)
    {
        uint32_t mesh_idx = ray_hit.mesh_index;
        uint32_t triangle_idx = ray_hit.triangle_index;
        const Triangle& triangle = scene->meshes[mesh_idx].triangles[triangle_idx];
        const Mesh& mesh = scene->meshes[mesh_idx];

        float u = ray_hit.triangle_u;
        float v = ray_hit.triangle_v;
        float w = 1.0f - u - v;

        glm::vec3 interpolated_point = ray_hit.ray.base + (ray_hit.ray.direction * ray_hit.distance);

        glm::vec3 n0 = scene->meshes[mesh_idx].normals[triangle.indices[0]];
        glm::vec3 n1 = scene->meshes[mesh_idx].normals[triangle.indices[1]];
        glm::vec3 n2 = scene->meshes[mesh_idx].normals[triangle.indices[2]];
        glm::vec3 interpolated_normal = glm::normalize((w * n0) + (u * n1) + (v * n2));
        glm::vec3 face_normal = interpolated_normal;
        if (glm::dot(ray_hit.ray.direction, face_normal) > 0.0f) {
            face_normal = -face_normal;
        }

        return HitData{
            triangle,
            &mesh,
            face_normal,
            interpolated_normal,
            interpolated_point
        };
    }

    float schlick(float r0, float costheta)
    {
        return r0 + (1.0f - r0) * glm::pow(1 - costheta, 5.0f);
    }

    float random_v1()
    {
        static std::atomic<uint64_t> thread_seed_counter{0};
        static thread_local std::mt19937_64 generator([]() {
            std::random_device rd;
            auto time_seed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            auto counter_seed = thread_seed_counter.fetch_add(1);
            auto hardware_seed = static_cast<uint64_t>(rd());
            uint64_t final_seed = hardware_seed ^ (counter_seed << 32) ^ time_seed;
            return std::mt19937_64(final_seed);
        }());

        static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);        
        return distribution(generator);

        // static thread_local std::mt19937_64 generator(std::random_device{}());
        // static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);        
        // return distribution(generator);

        // return ((float)rand() / (float)RAND_MAX);
    }

    glm::vec2 random_v2()
    {
        return glm::vec2(random_v1(), random_v1());
    }

    /*
        Returns true if "out" should be inserted into the hitpoint_buffer.
        This method causes side effects: writes into "result" 2D buffer
    */
    void ray_trace(Ray ray, HitPoint source, std::vector<HitPoint>& out, ssize_t depth_left = 5) {
        if (depth_left <= 0) {
            return;
        }

        if (source.pixel_weight.strength() < 0.001f) {
            return;
        }

        RayHit ray_hit = rt_engine->intersect(ray);
        if (!ray_hit.has_hit) return; 

        auto hit_data = this->resolve_hit_data(ray_hit);

        if (hit_data.mesh->pbr_material.is_emissive()) {
            FloatColor current_val = *(this->direct_emission_buffer.at(source.pixel_x, source.pixel_y));
            FloatColor new_val = source.pixel_weight * hit_data.mesh->pbr_material.emission;
            this->direct_emission_buffer.write(source.pixel_x, source.pixel_y, current_val + new_val);
            return;
        }

        Ray next_ray;
        next_ray.near = 0.001f;
        next_ray.base = hit_data.interpolated_point;

        bool has_diffuse = (1.0f - hit_data.mesh->pbr_material.metallic) * (1.0f - hit_data.mesh->pbr_material.transmission) > 0.0f;
        bool is_rough = hit_data.mesh->pbr_material.is_rough();

        if (has_diffuse || is_rough) {
            HitPoint clone = source;
            clone.material = &hit_data.mesh->pbr_material;
            clone.normal = glm::dot(hit_data.interpolated_normal, -ray.direction) > 0.0 ? hit_data.interpolated_normal : -hit_data.interpolated_normal;
            clone.direction = ray.direction;
            clone.position = hit_data.interpolated_point;
            
            out.push_back(clone);
        }

        // this happens only on non-rough (non-glossy) surfaces
        if (!is_rough) {
            auto specular_data = this->calculate_specular(hit_data, ray);
            
            // Reflection branch
            if (specular_data.fresnel.max_component() > 0.001f) {
                Ray refl_ray;
                refl_ray.base = hit_data.interpolated_point;
                refl_ray.direction = specular_data.reflection;
                refl_ray.near = 0.001f;

                HitPoint refl_hp = source;

                refl_hp.pixel_weight = 
                    refl_hp.pixel_weight 
                    * specular_data.fresnel;
                
                this->ray_trace(refl_ray, refl_hp, out, depth_left - 1);
            }

            // Tranmission branch
            if (hit_data.mesh->pbr_material.transmission > 0.0f) {
                // Transmission
                Ray refr_ray;
                refr_ray.base = hit_data.interpolated_point;
                refr_ray.direction = specular_data.refraction;
                refr_ray.near = 0.001f;

                HitPoint refr_hp = source;
                
                refr_hp.pixel_weight = 
                    refr_hp.pixel_weight 
                    * hit_data.mesh->pbr_material.base_color
                    * hit_data.mesh->pbr_material.transmission 
                    * (FloatColor(1.0f) - specular_data.fresnel);
                
                this->ray_trace(refr_ray, refr_hp, out, depth_left - 1);
            }
        }
    }

    class SpecularResult
    {
    public:
        FloatColor fresnel;
        glm::vec3 reflection;
        glm::vec3 refraction;
    };

    SpecularResult calculate_specular(const HitData& hit_data, const Ray& ray)
    {
        float costheta = glm::dot(-ray.direction, hit_data.interpolated_normal);
        auto outwards_normal = hit_data.interpolated_normal;
        float ref_coef;

        if (costheta > 0.0f) {
            // air -> material
            ref_coef = 1.0f / hit_data.mesh->pbr_material.ior;
        } else {
            // material -> air
            outwards_normal = -outwards_normal;
            ref_coef = hit_data.mesh->pbr_material.ior;
            costheta = -costheta;
        }

        // Thankfully R0 is symetric
        float n1 = hit_data.mesh->pbr_material.ior;
        FloatColor r0 = hit_data.mesh->pbr_material.calculate_R0();
        FloatColor schlick;// Schlick is PPB of reflection

        // TIR handling
        float sin2_theta_t = ref_coef * ref_coef * (1.0f - costheta * costheta);
        if (sin2_theta_t > 1.0f) {
            schlick = FloatColor(1.0f, 1.0f, 1.0f);
        } else {
            float schlick_costheta = costheta;

            // Source: https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
            // TLDR: this approximation breaks when n1 > n2
            if (ref_coef > 1.0f) {
                schlick_costheta = glm::sqrt(1.0f - sin2_theta_t);
            }

            schlick = Math::schlick_color(r0, schlick_costheta); 
        }

        return SpecularResult{
            .fresnel = schlick,
            .reflection = glm::reflect(ray.direction, outwards_normal),
            .refraction = glm::refract(ray.direction, outwards_normal, ref_coef)
        };
    }

public:
    ProgressivePhotonMapper(
        std::shared_ptr<AbstractRayTracingEngine> raytracing_engine,
        std::shared_ptr<Scene> scene,
        uint32_t width, 
        uint32_t height,
        float alpha,
        ssize_t photons_per_pass,
        float starting_radius,
        ssize_t photon_gather_limit
    ) : 
        rt_engine(raytracing_engine),
        scene(scene),
        result(Buffer2D<FloatColor>(width, height)), 
        direct_emission_buffer(Buffer2D<FloatColor>(width, height)),
        alpha(alpha),
        photons_per_pass(photons_per_pass),
        radius(starting_radius),
        photon_gather_limit(photon_gather_limit)
    {}

    void initialize() {
        // Build pixel map - multithread this

        // split map into (width/height) aspect ratio tiles
        ssize_t tile_width = 256;
        ssize_t tile_height = (result.get_height() * tile_width / result.get_width()); 

        std::queue<glm::ivec2> tiles;

        ssize_t tiles_vertical = (ssize_t)glm::ceil((float)result.get_width() / (float)tile_width);
        ssize_t tiles_horizontal = (ssize_t)glm::ceil((float)result.get_height() / (float)tile_height);

        for (ssize_t t_x = 0; t_x < tiles_vertical; t_x++ ) {
            for (ssize_t t_y = 0; t_y < tiles_horizontal; t_y++ ) {
                tiles.push({t_y, t_x});
            }
        }

        // Start reading from the queue (multithreaded)
        auto worker = [&tiles, tile_width, tile_height, this]() {
            // Do work until there is no more work left to do
            std::vector<HitPoint> local_buffer;
            while (true) {
                // Atomically take tile from queue
                this->tile_queue_mutex.lock();
                if (tiles.empty()) {
                    this->tile_queue_mutex.unlock();
                    break;
                }
                auto tile = tiles.front(); tiles.pop();
                this->tile_queue_mutex.unlock();

                // Do the work
                ssize_t start_x = tile.x * tile_width;
                ssize_t start_y = tile.y * tile_height;

                float hfov_tan = glm::tan(this->scene->camera->fov / 2.0f);

                for (ssize_t dx = 0; dx < tile_width; dx++) {
                    for (ssize_t dy = 0; dy < tile_height; dy++) {
                        float x = 2.0f * (((float)(start_x + dx) + 0.5f) / (float)this->result.get_width()) - 1.0f;
                        float y = 1.0f - 2.0f * (((float)(start_y + dy) + 0.5f) / (float)this->result.get_height()); // Reverse y axis

                        float width_to_height = (float)this->result.get_width() / (float)this->result.get_height();

                        glm::vec3 ray_direction = glm::normalize(
                            this->scene->camera->get_forward() + 
                            hfov_tan * this->scene->camera->get_right() * x * width_to_height +
                            hfov_tan * this->scene->camera->get_up() * y
                        );
                        
                        Ray ray;
                        ray.direction = ray_direction;
                        ray.base = this->scene->camera->position;
                        ray.is_coherent = true;
                        ray.near = 0.00001f;

                        HitPoint hitpoint;
                        hitpoint.pixel_x = start_x + dx;
                        hitpoint.pixel_y = start_y + dy;
                        hitpoint.radius_squared = this->radius * this->radius;
                        hitpoint.photon_count = 0.0f;
                        this->ray_trace(ray, hitpoint, local_buffer);
                    } 
                }
            }
            this->hitpoint_buffer_mutex.lock();
            for (auto local_hp : local_buffer) {
                hitpoint_buffer.push_back(local_hp);
            }
            this->hitpoint_buffer_mutex.unlock();
        };

        ssize_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (uint32_t i = 0; i < num_threads; i++) {
            threads.emplace_back(worker);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        log_info("[PPM]: Finished initial ray trace pass");
    }

    void cast_photon(PhotonRay photon_ray, std::vector<Photon>& photon_vector)
    {
        FloatColor photon_emission = photon_ray.photon_emission;
        ssize_t bounce_limit = 32; // not very Physically accurate of me

        Ray ray = photon_ray.ray;
        RayHit ray_hit = rt_engine->intersect(ray);
        
        while (ray_hit.has_hit) {
            if (bounce_limit == 0) break;
            bounce_limit--;
            auto hit_data = this->resolve_hit_data(ray_hit);
            auto specular_data = this->calculate_specular(hit_data, ray);
            Ray next_ray;

            float p_reflect = std::clamp(specular_data.fresnel.max_component(), 0.001f, 0.999f);
            float p_enter = 1.0f - p_reflect;

            float p_transmit = p_enter * hit_data.mesh->pbr_material.transmission * (1.0f - hit_data.mesh->pbr_material.metallic);
            float p_diffuse = p_enter * (1.0f - hit_data.mesh->pbr_material.transmission) * (1.0f - hit_data.mesh->pbr_material.metallic);

            float choice = random_v1();

            next_ray.base = hit_data.interpolated_point;
            next_ray.near = 0.001f;

            glm::vec3 outwards_normal = hit_data.interpolated_normal;
            if (glm::dot(outwards_normal, ray.direction) > 0.0f) {
                outwards_normal = -outwards_normal;
            }

            if (choice < p_reflect) {
                // Reflection
                float ggx_weight = 1.0f;
                if (hit_data.mesh->pbr_material.is_rough()) {
                    photon_vector.push_back(Photon{
                        .emission = photon_emission,
                        .intersection = hit_data.interpolated_point,
                        .direction = ray.direction
                    });
                    glm::vec3 view_dir = -ray.direction;
                    next_ray.direction = Math::ggx_sample(outwards_normal, view_dir, hit_data.mesh->pbr_material.roughness, random_v2());
                    // Appereantly it's important to make sure the photon was not directed under the surface
                    if (glm::dot(next_ray.direction, outwards_normal) <= 0.0f) {
                        break; 
                    }

                    // GGX Throughput
                    glm::vec3 V = view_dir;
                    glm::vec3 L = next_ray.direction;
                    glm::vec3 H = glm::normalize(V + L);

                    float n_dot_v = std::max(0.001f, glm::dot(outwards_normal, V));
                    float n_dot_l = std::max(0.001f, glm::dot(outwards_normal, L));
                    float n_dot_h = std::max(0.001f, glm::dot(outwards_normal, H));
                    float v_dot_h = std::max(0.001f, glm::dot(V, H));

                    float G = Math::ggx_geometry_smith(outwards_normal, V, L, hit_data.mesh->pbr_material.roughness);
                    ggx_weight = (G * v_dot_h) / (n_dot_v * n_dot_h);
                } else {
                    next_ray.direction = specular_data.reflection;
                }
                photon_emission = photon_emission * ggx_weight * specular_data.fresnel * (1.0f / p_reflect);
            } else if (choice < p_reflect + p_transmit) {
                // We transmit to the other side
                next_ray.direction = specular_data.refraction;
                FloatColor energy_left = FloatColor(1.0f) - specular_data.fresnel;
                photon_emission = photon_emission * hit_data.mesh->pbr_material.base_color * energy_left * (1.0f / p_enter);

            } else if (choice < p_reflect + p_transmit + p_diffuse) {
                // Mate (diffuse)
                photon_vector.push_back(Photon{
                    .emission = photon_emission,
                    .intersection = hit_data.interpolated_point,
                    .direction = ray.direction
                });

                FloatColor energy_left = FloatColor(1.0f) - specular_data.fresnel;

                photon_emission = photon_emission * hit_data.mesh->pbr_material.base_color * energy_left * (1.0f / p_enter);
                next_ray.direction = Math::sample_cosine_hemisphere(outwards_normal, random_v2());

            } else {
                // Absorb the photon
                break;
            }

            ray_hit = rt_engine->intersect(next_ray);
            ray = next_ray;
        }
    }

    float calculate_next_radius_squared(float old_radius_squared, float n, float m)
    {
        return old_radius_squared * (n + this->alpha * m) / (n + m);
    }

    FloatColor calculate_new_flux(FloatColor old_flux, FloatColor next_flux, float n, float m)
    {
        return (old_flux + next_flux) * (n + this->alpha * m) * (1.0f / (n + m));
    }

    void update_pixel_data(ssize_t start_from, ssize_t end_at)
    {
        Buffer2D<FloatColor> temp_buffer(this->result.get_width(), this->result.get_height());
        std::vector<Photon> gathered_photons;

        // no multithreading this time
        for (ssize_t i = start_from; i < end_at; i++) {
            auto& hitpoint = hitpoint_buffer[i];
            this->photon_map.search(hitpoint.position, hitpoint.radius_squared, gathered_photons);
            float next_radius_squared = calculate_next_radius_squared(hitpoint.radius_squared, hitpoint.photon_count, gathered_photons.size());
            FloatColor old_flux = hitpoint.accumulated_flux;

            // Calculate ne
            FloatColor new_flux = FloatColor(0.0f, 0.0f, 0.0f);
            for (auto& photon : gathered_photons) {
                // Check if this photon even is interesting to us
                glm::vec3 L = -photon.direction;
                glm::vec3 V = -hitpoint.direction;
                glm::vec3 H = glm::normalize(V + L);

                float n_dot_v = std::max(0.0f, glm::dot(hitpoint.normal, V));
                float n_dot_l = std::max(0.0f, glm::dot(hitpoint.normal, L));

                if (n_dot_l <= 0.0f || n_dot_v <= 0.0f) continue;

                // Cook-Torrance BRDF with Smith-Schlick GGX

                float D = Math::ggx_distribution(hitpoint.normal, H, hitpoint.material->roughness);
                float G = Math::ggx_geometry_smith(hitpoint.normal, V, L, hitpoint.material->roughness);

                float v_dot_h = std::max(0.0f, glm::dot(V, H));
                FloatColor r0 = hitpoint.material->calculate_R0();
                FloatColor F_microfacet = Math::schlick_color(r0, v_dot_h);
                
                FloatColor brdf_glossy = FloatColor(0.0f, 0.0f, 0.0f);
                // if (hitpoint.material->is_rough()) {
                    FloatColor numerator = F_microfacet * D * G;
                    float denominator = 4.0f * n_dot_v * n_dot_l;
                    brdf_glossy = numerator * (1.0f / std::max(denominator, 0.001f));
                // }
                
                // Energy left post fresnel
                FloatColor energy_left = FloatColor(1.0f) - F_microfacet;

                // Metals do not have Mate. Glass DOES not have Mate
                float diffuse_ratio = (1.0f - hitpoint.material->metallic) * (1.0f - hitpoint.material->transmission);

                FloatColor brdf_diffuse = hitpoint.material->base_color * energy_left * diffuse_ratio * (1.0f / glm::pi<float>());

                FloatColor brdf = brdf_diffuse + brdf_glossy;
                new_flux = new_flux + brdf * photon.emission;
            }

            FloatColor final_flux = this->calculate_new_flux(old_flux, new_flux, hitpoint.photon_count, gathered_photons.size());
            FloatColor final_radiance = final_flux * (1.0f / (glm::pi<float>() * next_radius_squared * this->total_photons_emitted_so_far));

            // Finally we can write color

            FloatColor old_value = *(temp_buffer.at(hitpoint.pixel_x, hitpoint.pixel_y));
            
            // // Standard
            temp_buffer.write(hitpoint.pixel_x, hitpoint.pixel_y, old_value + final_radiance * hitpoint.pixel_weight);
            this->result.write(hitpoint.pixel_x, hitpoint.pixel_y, old_value + final_radiance * hitpoint.pixel_weight);
            
            // Debug raytracer
            // temp_buffer.write(hitpoint.pixel_x, hitpoint.pixel_y, old_value + hitpoint.material->base_color * hitpoint.pixel_weight);
            // this->result.write(hitpoint.pixel_x, hitpoint.pixel_y, old_value + hitpoint.material->base_color * hitpoint.pixel_weight);

            // Save new data to hitpoint

            hitpoint.accumulated_flux = final_flux;
            hitpoint.photon_count += this->alpha * (float)gathered_photons.size();
            hitpoint.radius_squared = next_radius_squared;
        }
    }

    void prepare_photon_ray_vector(std::vector<PhotonRay>& photon_ray_vector)
    {
        float previous_r = 0.0f;
        float total_emissive_area = scene->emissive_triangles.get_total_emissive_area();
        float emissive_area_prop = 0.0f;

        for (ssize_t t_ptr = 0; t_ptr < scene->emissive_triangles.container.size(); t_ptr++) {
            // 0. For each emissive triangle we calculate how many photons it should shoot based on area
            const auto& trig = scene->emissive_triangles.container[t_ptr];
            float area = (trig.r - previous_r);
            previous_r = trig.r;
            float contribution = area / total_emissive_area;
            emissive_area_prop += contribution;
            uint32_t photons_count = this->photons_per_pass * contribution;

            // We do not want to divide this! Emission here is actually flux - so it's not normalized yet 
            // - this is usefull to calculate total flux in hitpoints
            FloatColor emission_per_photon = (trig.owner_mesh.pbr_material.emission * total_emissive_area * glm::pi<float>());

            for (ssize_t p_cnt = 0; p_cnt < photons_count; p_cnt++) {
                FloatColor photon_emission = emission_per_photon;
                // Shoot photons from this triangle
                
                // 1. Get random point
                auto point = Math::sample_point_on_triangle(trig.trig[0], trig.trig[1], trig.trig[2], random_v2());

                // 2. random hemisphere direction
                auto direction = Math::sample_cosine_hemisphere(trig.normal, random_v2());

                // 3. Make ray
                auto ray = Ray{.base = point, .direction = direction, .near = 0.0001f};
                photon_ray_vector.push_back(PhotonRay{photon_emission, ray});
            }
        }
    }

    void run_next_photon_map_step() {
        // 1. Fire photons
        // first split photons into groups again so we can use multithreading
        std::vector<Photon> photon_vector;
        std::vector<PhotonRay> photon_ray_vector;
        // This step should be fast enough, but maybe in the future I could also parallelize this
        prepare_photon_ray_vector(photon_ray_vector);

        ssize_t photon_batch_size = 10000;

        ssize_t max_batch_count = glm::ceil((float)photon_ray_vector.size() / (float)photon_batch_size);
        ssize_t last_batch = 0;

        auto photon_thread = [this, photon_batch_size, &last_batch, max_batch_count, &photon_ray_vector, &photon_vector]() {
            std::vector<Photon> photon_batch;            
            while (true) {
                this->photon_queue_mutex.lock();
                if (last_batch >= max_batch_count) {
                    this->photon_queue_mutex.unlock();
                    break;
                }
                int my_batch_idx = last_batch++;
                this->photon_queue_mutex.unlock();

                ssize_t start = my_batch_idx * photon_batch_size;
                ssize_t end = std::min((ssize_t)(my_batch_idx+1) * photon_batch_size, (ssize_t)photon_ray_vector.size());

                for (ssize_t i = start; i<end; i++) {
                    Photon resulting_photon;
                    this->cast_photon(photon_ray_vector[i], photon_batch);
                } 
            }
            // make lock on the photon vector
            photon_vector_mutex.lock();
            for (auto photon : photon_batch) {
                photon_vector.push_back(photon);
            }
            photon_vector_mutex.unlock();
        };

        ssize_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (uint32_t i = 0; i < num_threads; i++) {
            threads.emplace_back(photon_thread);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Rebuild photon map
        this->photon_map = PhotonMap(PhotonMap(std::move(photon_vector)));

        log_info("[PPM]: Finished building new photon map -  ", this->photon_map.get_size(), " total photons.");
        total_photons_emitted_so_far += photons_per_pass;

        
        // 2. Gather pixel data - multithread this aswel
        ssize_t pixels_per_batch = 100000;
        ssize_t total_pixels = this->hitpoint_buffer.size();
        ssize_t max_batches = glm::ceil( (float)total_pixels / (float)pixels_per_batch );
        ssize_t last_pixel_batch = 0;

        log_info("Pixels to process: ", total_pixels);

        auto pixel_data_worker = [this, &last_pixel_batch, max_batches, pixels_per_batch, total_pixels]() {
            while (true) {
                this->pixel_queue_mutex.lock();
                if (last_pixel_batch >= max_batches) {
                    this->pixel_queue_mutex.unlock();
                    break;
                }
                ssize_t my_batch = last_pixel_batch++;
                this->pixel_queue_mutex.unlock();

                ssize_t start_at = my_batch * pixels_per_batch;
                ssize_t end_at = std::min((my_batch + 1) * pixels_per_batch, total_pixels);

                this->update_pixel_data(start_at, end_at);
            }  
        };

        threads.clear();
        for (uint32_t i = 0; i < num_threads; i++) {
            threads.emplace_back(pixel_data_worker);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Copy direct emission buffer
        for (ssize_t x = 0; x < this->direct_emission_buffer.get_width(); x++ ) {
            for (ssize_t y = 0; y < this->direct_emission_buffer.get_height(); y++ ) {
                FloatColor value = *this->direct_emission_buffer.at(x, y);
                if (value.strength() > 0.0f) {
                    FloatColor old_value = *this->result.at(x, y);
                    this->result.write(x, y, value + old_value);
                }

                this->direct_emission_buffer.write(x, y, FloatColor(0, 0, 0));
            }
        }

        log_info("[PPM]: Finished PPM step.");
    };

    const Buffer2D<FloatColor>& get_result() {
        return result;
    };
};