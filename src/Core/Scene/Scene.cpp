#include <Core/Scene/Scene.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <Misc/Math/Math.hpp>
#include <algorithm>
#include <Infrastructure/Logger/Logger.hpp>

Scene::~Scene() = default;

WeightedEmissiveTriangleList::WeightedEmissiveTriangleList() {}

void WeightedEmissiveTriangleList::insert(Mesh& owner_mesh) {
    for (auto& trig : owner_mesh.triangles) {
        glm::vec3 triangle[3] = {
            owner_mesh.vertices[trig.indices[0]],
            owner_mesh.vertices[trig.indices[1]],
            owner_mesh.vertices[trig.indices[2]],
        };

        this->insert(owner_mesh, triangle);
    }
}

void WeightedEmissiveTriangleList::insert(Mesh& owner_mesh, glm::vec3 trig[3]) {
    auto edge1 = trig[1] - trig[0];
    auto edge2 = trig[2] - trig[0];
    float area = 0.5f * glm::length(glm::cross(edge1, edge2));
    if (area <= 0.000001f) {
        return; 
    }

    glm::vec3 geometric_normal = glm::normalize(glm::cross(edge1, edge2));

    float current_start = 0.0f;
    if (this->container.size() > 0) {
        current_start = this->container.back().r;
    }

    WeightedEmissiveTriangleListElement element = WeightedEmissiveTriangleListElement{
        .r = current_start + area,
        .owner_mesh = owner_mesh,
        .trig = {trig[0], trig[1], trig[2]},
        .normal =  geometric_normal
    };
    this->container.push_back(element);
};

// Binary search
const WeightedEmissiveTriangleListElement& WeightedEmissiveTriangleList::get_random(float random_float) {
    assert(this->container.size() > 0);
    float total_area = this->container.back().r; 
    float random_dart = random_float * total_area;
    auto it = std::lower_bound(
        this->container.begin(), 
        this->container.end(), 
        random_dart,
        [](const WeightedEmissiveTriangleListElement& element, float value) {
            return element.r < value;
        }
    );
    if (it == this->container.end()) {
        return this->container.back();
    }
    return *it;
};

float WeightedEmissiveTriangleList::get_total_emissive_area()
{
    if (this->container.empty()) return 0.0f;
    return this->container.back().r;
}

void WeightedEmissiveTriangleList::debug_log_content()
{
    log_info("START WeightedEmissiveTriangleList CONTENT");
    for (auto& el : this->container) {
        log_info(el.r);
    }
    log_info("END WeightedEmissiveTriangleList CONTENT");
};

const WeightedEmissiveTriangleListElement& Scene::get_weighted_random_emissivie_triangle(float random_float) {
    return this->emissive_triangles.get_random(random_float);
}