#pragma once

#include <vector>
#include <iostream>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include <vector>
#include <iostream>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>

namespace HavranTree
{
    struct AABB {
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::infinity());
        glm::vec3 max = glm::vec3(-std::numeric_limits<float>::infinity());

        void expand(const glm::vec3& p) {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        void expand(const AABB& other) {
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }

        float surface_area() const {
            glm::vec3 d = max - min;
            if (d.x < 0 || d.y < 0 || d.z < 0) return 0.0f;
            return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
        }
    };

    struct Triangle {
        glm::vec3 v0, v1, v2;
        AABB get_aabb() const {
            AABB box;
            box.expand(v0); box.expand(v1); box.expand(v2);
            return box;
        }
    };

    struct Ray {
        glm::vec3 origin;
        glm::vec3 dir;
        glm::vec3 inv_dir;
        Ray(glm::vec3 o, glm::vec3 d) : origin(o), dir(d) {
            inv_dir = glm::vec3(
                d.x == 0.0f ? 1e-6f : 1.0f / d.x,
                d.y == 0.0f ? 1e-6f : 1.0f / d.y,
                d.z == 0.0f ? 1e-6f : 1.0f / d.z
            );
        }
    };

    struct Hit {
        float t = std::numeric_limits<float>::infinity();
        uint32_t triangle_idx = static_cast<uint32_t>(-1);
        glm::vec2 uv;
    };

    class RSAKDTree {
    private:
        static constexpr float C_TS = 1.0f; 
        static constexpr float C_IT = 1.5f; 
        static constexpr int MAX_DEPTH = 24; 

        struct Node {
            int32_t left = -1;  
            int32_t right = -1; 
            uint8_t axis = 3;       
            float split_pos = 0.0f; 
            std::vector<uint32_t> triangles; 
        };

        struct Edge {
            float pos;
            int type;
            
            bool operator<(const Edge& o) const {
                if (pos == o.pos) return type > o.type;
                return pos < o.pos;
            }
        };

        std::vector<Node> tree;
        const std::vector<Triangle>* original_triangles; 
        AABB root_bbox;

        int32_t build(const AABB& node_bbox, const std::vector<uint32_t>& tri_indices, int depth) 
        {
            int32_t index = tree.size();
            tree.push_back(Node{});

            int N = tri_indices.size();
            float C_leaf = N * C_IT;

            if (N <= 2 || depth >= MAX_DEPTH) {
                tree[index].axis = 3; 
                tree[index].triangles = tri_indices;
                return index;
            }

            float best_cost = C_leaf;
            int best_axis = -1;
            float best_split_pos = 0.0f;
            float SA_V = node_bbox.surface_area(); 

            for (int axis = 0; axis < 3; axis++) {
                std::vector<Edge> edges;
                edges.reserve(2 * N);
                
                for (uint32_t i = 0; i < N; i++) {
                    AABB box = (*original_triangles)[tri_indices[i]].get_aabb();
                    float e_min = glm::max(box.min[axis], node_bbox.min[axis]);
                    float e_max = glm::min(box.max[axis], node_bbox.max[axis]);
                    
                    edges.push_back({e_min, 0});
                    edges.push_back({e_max, 1});
                }
                
                std::sort(edges.begin(), edges.end());

                int N_L = 0;
                int N_R = N;

                for (size_t i = 0; i < edges.size(); i++) {
                    if (edges[i].type == 1) N_R--; 

                    float split_pos = edges[i].pos;

                    if (split_pos > node_bbox.min[axis] && split_pos < node_bbox.max[axis]) {
                        AABB box_L = node_bbox; box_L.max[axis] = split_pos;
                        AABB box_R = node_bbox; box_R.min[axis] = split_pos;
                        float SA_L = box_L.surface_area();
                        float SA_R = box_R.surface_area();

                        float current_cost = C_TS + (SA_L / SA_V) * N_L * C_IT + (SA_R / SA_V) * N_R * C_IT;

                        if (current_cost < best_cost) {
                            best_cost = current_cost;
                            best_axis = axis;
                            best_split_pos = split_pos;
                        }
                    }

                    if (edges[i].type == 0) N_L++; 
                }
            }

            if (best_axis == -1) {
                tree[index].axis = 3; 
                tree[index].triangles = tri_indices;
                return index;
            }

            std::vector<uint32_t> left_tris, right_tris;
            for (uint32_t idx : tri_indices) {
                AABB tri_box = (*original_triangles)[idx].get_aabb();
                if (tri_box.min[best_axis] <= best_split_pos) left_tris.push_back(idx);
                if (tri_box.max[best_axis] >= best_split_pos) right_tris.push_back(idx);
            }

            tree[index].axis = best_axis;
            tree[index].split_pos = best_split_pos;

            AABB left_bbox = node_bbox; left_bbox.max[best_axis] = best_split_pos;
            AABB right_bbox = node_bbox; right_bbox.min[best_axis] = best_split_pos;

            int32_t left_child = this->build(left_bbox, left_tris, depth + 1);
            int32_t right_child = this->build(right_bbox, right_tris, depth + 1);

            tree[index].left = left_child;
            tree[index].right = right_child;

            return index;
        }

        void intersect_node(int32_t node_idx, const Ray& ray, float t_min, float t_max, Hit& hit) const {
            if (node_idx == -1) return;

            if (hit.t < t_min) return;

            const Node& node = tree[node_idx];

            if (node.axis == 3) { // Liść
                for (uint32_t tri_idx : node.triangles) {
                    float t; glm::vec2 uv;
                    if (intersect_triangle(ray, (*original_triangles)[tri_idx], t, uv)) {
                        if (t < hit.t) {
                            hit.t = t;
                            hit.triangle_idx = tri_idx;
                            hit.uv = uv;
                        }
                    }
                }
                return;
            }

            float t_split = (node.split_pos - ray.origin[node.axis]) * ray.inv_dir[node.axis];

            int32_t first = (ray.dir[node.axis] >= 0.0f) ? node.left : node.right;
            int32_t second = (ray.dir[node.axis] >= 0.0f) ? node.right : node.left;

            if (t_split <= t_min) {
                intersect_node(second, ray, t_min, t_max, hit);
            } else if (t_split >= t_max) {
                intersect_node(first, ray, t_min, t_max, hit);
            } else {
                intersect_node(first, ray, t_min, t_split, hit);
                if (hit.t >= t_split) {
                    intersect_node(second, ray, t_split, t_max, hit);
                }
            }
        }

        bool intersect_aabb(const AABB& box, const Ray& ray, float& t_min, float& t_max) const {
            t_min = 0.0f;
            t_max = std::numeric_limits<float>::infinity();

            for (int i = 0; i < 3; ++i) {
                float t1 = (box.min[i] - ray.origin[i]) * ray.inv_dir[i];
                float t2 = (box.max[i] - ray.origin[i]) * ray.inv_dir[i];

                t_min = glm::max(t_min, glm::min(glm::min(t1, t2), t_max));
                t_max = glm::min(t_max, glm::max(glm::max(t1, t2), t_min));
            }
            return t_max >= glm::max(t_min, 0.0f);
        }

    public:
        RSAKDTree(const std::vector<Triangle>& list) {
            if (list.empty()) return;
            original_triangles = &list;
            
            std::vector<uint32_t> initial_indices;
            initial_indices.reserve(list.size());
            
            for(uint32_t i=0; i<list.size(); ++i) {
                root_bbox.expand(list[i].get_aabb());
                initial_indices.push_back(i);
            }

            this->build(root_bbox, initial_indices, 0);
        }

        Hit trace(const Ray& ray) const {
            Hit hit;
            float t_min, t_max;
            
            if (intersect_aabb(root_bbox, ray, t_min, t_max)) {
                intersect_node(0, ray, t_min, t_max, hit);
            }
            return hit;
        }

        // Möller–Trumbore
        static bool intersect_triangle(const Ray& ray, const Triangle& tri, float& t_out, glm::vec2& uv_out) {
            const float EPSILON = 0.0000001f;
            glm::vec3 edge1 = tri.v1 - tri.v0;
            glm::vec3 edge2 = tri.v2 - tri.v0;
            glm::vec3 h = glm::cross(ray.dir, edge2);
            float a = glm::dot(edge1, h);

            if (a > -EPSILON && a < EPSILON) return false; 
            
            float f = 1.0f / a;
            glm::vec3 s = ray.origin - tri.v0;
            float u = f * glm::dot(s, h);
            if (u < 0.0f || u > 1.0f) return false;

            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(ray.dir, q);
            if (v < 0.0f || u + v > 1.0f) return false;

            float t = f * glm::dot(edge2, q);
            if (t > EPSILON) {
                t_out = t;
                uv_out = glm::vec2(u, v);
                return true;
            }
            return false;
        }
    };
}