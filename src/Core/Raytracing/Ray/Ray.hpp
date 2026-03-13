#pragma once
#include <glm/glm.hpp>
#include <embree4/rtcore.h>

class Ray 
{
public:
    glm::vec3 base;
    glm::vec3 direction;

    RTCRay get_embree_ray(float tnear = 0.0001f, float tfar = std::numeric_limits<float>::infinity()) const;

    RTCRayHit get_embree_ray_hit(float tnear = 0.0001f, float tfar = std::numeric_limits<float>::infinity()) const;
};