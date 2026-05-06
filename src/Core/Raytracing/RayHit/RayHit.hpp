#pragma once
#include "Core/Raytracing/Ray/Ray.hpp"
#include <memory>

class RayHit
{
public:
    Ray ray;
    bool has_hit;
    float distance;
    uint32_t triangle_index;
    uint32_t mesh_index;
    float triangle_u;
    float triangle_v;
};
