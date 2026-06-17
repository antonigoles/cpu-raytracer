#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>

class Photon
{
public:
    FloatColor emission;
    glm::vec3 intersection;
    glm::vec3 direction;
};

class PhotonMap {
private:
    std::vector<Photon> photons;
    struct StackNode {
        int32_t start;
        int32_t end;
        int32_t depth;
    };

    void build_recursive(int32_t start, int32_t end, int32_t depth) {
        if (start > end) return;

        int32_t mid = start + (end - start) / 2;
        int32_t axis = depth % 3;

        std::nth_element(
            photons.begin() + start,
            photons.begin() + mid,
            photons.begin() + end + 1,
            [axis](const Photon& a, const Photon& b) {
                return a.intersection[axis] < b.intersection[axis];
            }
        );

        build_recursive(start, mid - 1, depth + 1);
        build_recursive(mid + 1, end, depth + 1);
    }

public:
    PhotonMap() = default;

    PhotonMap(std::vector<Photon>&& list) : photons(std::move(list)) {
        if (!photons.empty()) {
            build_recursive(0, photons.size() - 1, 0);
        }
    }

    ssize_t get_size() const
    {
        return this->photons.size();
    }

    void search(const glm::vec3& point, float max_radius2, std::vector<Photon>& out_photons) const {
        out_photons.clear();

        if (photons.empty()) return;

        StackNode stack[64];
        int32_t stack_ptr = 0;

        stack[stack_ptr++] = { 0, (int32_t)photons.size() - 1, 0 };

        while (stack_ptr > 0) {
            auto node = stack[--stack_ptr];
            
            if (node.start > node.end) continue;

            int32_t mid = node.start + (node.end - node.start) / 2;
            const Photon& p = photons[mid];
            int32_t axis = node.depth % 3;

            float dist2 = glm::distance2(point, p.intersection);
            if (dist2 < max_radius2) {
                out_photons.push_back(p);
            }

            float diff = point[axis] - p.intersection[axis];

            int32_t left_start = node.start;
            int32_t left_end = mid - 1;
            int32_t right_start = mid + 1;
            int32_t right_end = node.end;

            if (diff < 0.0f) {
                if (diff * diff < max_radius2 && right_start <= right_end) {
                    stack[stack_ptr++] = { right_start, right_end, node.depth + 1 };
                }
                if (left_start <= left_end) {
                    stack[stack_ptr++] = { left_start, left_end, node.depth + 1 };
                }
            } else {
                if (diff * diff < max_radius2 && left_start <= left_end) {
                    stack[stack_ptr++] = { left_start, left_end, node.depth + 1 };
                }
                if (right_start <= right_end) {
                    stack[stack_ptr++] = { right_start, right_end, node.depth + 1 };
                }
            }
        }
    }
};