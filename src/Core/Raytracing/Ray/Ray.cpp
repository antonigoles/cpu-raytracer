#include <Core/Raytracing/Ray/Ray.hpp>

RTCRay Ray::get_embree_ray(float tnear, float tfar) const
{
    RTCRay query;

    query.org_x = base.x;
    query.org_y = base.y;
    query.org_z = base.z;

    query.dir_x = direction.x;
    query.dir_y = direction.y;
    query.dir_z = direction.z;

    query.tnear = tnear;
    query.tfar = tfar;

    query.mask = -1;
    query.flags = 0;

    return query;
}

RTCRayHit Ray::get_embree_ray_hit(float tnear, float tfar) const
{
    RTCRayHit query;

    query.ray.org_x = base.x;
    query.ray.org_y = base.y;
    query.ray.org_z = base.z;

    query.ray.dir_x = direction.x;
    query.ray.dir_y = direction.y;
    query.ray.dir_z = direction.z;

    query.ray.tnear = tnear;
    query.ray.tfar = tfar;

    query.ray.mask = -1;
    query.ray.flags = 0;

    query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    return query;
}
