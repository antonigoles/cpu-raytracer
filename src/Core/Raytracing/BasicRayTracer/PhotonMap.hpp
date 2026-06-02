#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>
#include <iostream>
#include <queue>

class Photon
{
public:
    FloatColor emission;
    glm::vec3 intersection;
    glm::vec3 direction;
};

class PhotonMap {
private:
    struct Node {
        uint64_t photon_idx;
        glm::vec3 point;
        int32_t left;
        int32_t right;
        uint8_t axis;
    };

    static constexpr int K = 3;

    std::vector<Node> tree;
    std::vector<Photon> photons;

    inline float distance2(const glm::vec3& a, const glm::vec3& b) {
        return glm::distance2(a, b);
    };

    inline uint32_t get_depth_from_index(const uint32_t& index) {
        return std::bit_width(index) - 1;
    };

    bool null_vec(const glm::vec3& v) {
        // IEEE 754 trick
        return v[0] != v[0];
    }

    int32_t build(
        typename std::vector<Photon>::iterator start, 
        typename std::vector<Photon>::iterator end,
        uint8_t axis    
    ) {
        int32_t size = std::distance(start, end);
        if (size <= 0) return -1;

        int32_t index = tree.size();
        int32_t mid = size / 2;

        std::nth_element(
            start,
            start + mid,
            end,
            [axis](const Photon& a, const Photon& b) {
                return a.intersection[axis] < b.intersection[axis];
            }
        );

        photons.push_back(*(start + mid));

        tree.push_back(Node{ photons.size() - 1, (start + mid)->intersection, -1, -1, axis });

        int32_t left_child = this->build(start, start + mid, (axis + 1) % K);
        int32_t right_child = this->build(start + mid + 1, end, (axis + 1) % K);

        tree[index].left = left_child;
        tree[index].right = right_child;

        return index;
    }

public:
    PhotonMap() {
        // empty
    }

    PhotonMap(std::vector<Photon> list) {
        assert(list.size() > 0);
        photons.resize(list.size());
        this->build(list.begin(), list.end(), 0);
    }

    void print_tree_structure() {
        // print from last
        for (int j = 0; j < tree.size(); j++) {
            if (this->null_vec(tree[j].point)) continue;
            std::cout << j << ": ";
            for (int i = 0; i < K; i++) {
                std::cout << tree[j].point[i] << " ";
            }
            std::cout << "\n";
        }
    }

    std::vector<Photon> search(
        const glm::vec3& point, int32_t N, float max_radius2
    ) const {
        std::vector<Photon> result;
        // For speed let's assume tree is never empty

        std::priority_queue<std::pair<float, uint64_t>> pq;
        this->internal_search(point, 0, N, pq, max_radius2);

        result.reserve(pq.size());
        while (!pq.empty()) {
            result.push_back(photons[pq.top().second]);
            pq.pop();
        }

        // std::reverse(result.begin(), result.end());

        return result;
    }

private:
    void internal_search(
        const glm::vec3& point, 
        int32_t node_idx,
        int32_t N,
        std::priority_queue<std::pair<float, uint64_t>>& pq,
        float max_radius2
    ) const {
        const auto& node = tree[node_idx]; 

        float dist = glm::distance2(point, node.point); 
        if (dist < max_radius2) {
            if (pq.size() < N) {
                pq.push({dist, node.photon_idx});
            } else if (dist < pq.top().first) {
                pq.pop();
                pq.push({dist, node.photon_idx});
            }
        }

        if (node.left == -1 && node.right == -1) {
            return; 
        }

        float diff = point[node.axis] - node.point[node.axis];
        
        int32_t closer = diff < 0 ? node.left : node.right; 
        int32_t further = diff < 0 ? node.right : node.left; 

        if (closer != -1) {
            internal_search(point, closer, N, pq, max_radius2);
        }

        float search_radius_sq = (pq.size() == N) ? pq.top().first : max_radius2;
        if (further != -1 && (diff * diff < search_radius_sq)) {
            internal_search(point, further, N, pq, max_radius2);
        }
    }
};