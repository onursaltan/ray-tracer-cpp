#ifndef UV_MAPPING_HPP
#define UV_MAPPING_HPP

#include "math.hpp"
#include "primitives.hpp"

struct UVCoord
{
    float u, v;

    UVCoord() : u(0), v(0) {}
    UVCoord(float u_, float v_) : u(u_), v(v_) {}
};

inline UVCoord getSphereUV(const Vec3 &point, const Vec3 &center, float radius)
{
    Vec3 p = normalize(point - center);
    float phi = std::atan2(p.z, p.x);
    float theta = std::asin(p.y);

    UVCoord uv;
    uv.u = (phi + 3.14159265359f) / (2.0f * 3.14159265359f);
    uv.v = (theta + 3.14159265359f / 2.0f) / 3.14159265359f;
    return uv;
}

inline UVCoord getPlaneUV(const Vec3 &point, const Vec3 &planeOrigin, float size)
{
    Vec3 local = point - planeOrigin;
    UVCoord uv;
    uv.u = (local.x / size + 0.5f);
    uv.v = (local.z / size + 0.5f);
    return uv;
}

inline UVCoord getCubeUV(const Vec3 &point, const Vec3 &cubeCenter, float size, const Vec3 &normal)
{
    Vec3 local = point - cubeCenter;
    UVCoord uv;

    float absX = std::abs(normal.x);
    float absY = std::abs(normal.y);
    float absZ = std::abs(normal.z);

    if (absX > absY && absX > absZ)
    {
        uv.u = (local.z / size + 0.5f);
        uv.v = (local.y / size + 0.5f);
    }
    else if (absY > absX && absY > absZ)
    {
        uv.u = (local.x / size + 0.5f);
        uv.v = (local.z / size + 0.5f);
    }
    else
    {
        uv.u = (local.x / size + 0.5f);
        uv.v = (local.y / size + 0.5f);
    }

    return uv;
}

#endif
