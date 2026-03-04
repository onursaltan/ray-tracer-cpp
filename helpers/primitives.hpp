#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include "math.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

constexpr float RAY_EPSILON = 1e-6f;

constexpr float RAY_ORIGIN_OFFSET = 1e-4f;

struct Ray
{
    Vec3 origin, dir;
};

struct Vec2
{
    float u, v;
    Vec2(float u_ = 0, float v_ = 0) : u(u_), v(v_) {}
};

struct Triangle
{
    Vec3 v0, v1, v2;
    Vec3 n0, n1, n2;
    Vec2 uv0, uv1, uv2;
    bool hasVertexNormals;
    bool hasUVs;

    Triangle() : v0(), v1(), v2(), n0(), n1(), n2(),
                 uv0(), uv1(), uv2(), hasVertexNormals(false), hasUVs(false) {}

    Triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c)
        : v0(a), v1(b), v2(c), n0(), n1(), n2(),
          uv0(), uv1(), uv2(), hasVertexNormals(false), hasUVs(false) {}

    Triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c,
             const Vec3 &na, const Vec3 &nb, const Vec3 &nc)
        : v0(a), v1(b), v2(c), n0(na), n1(nb), n2(nc),
          uv0(), uv1(), uv2(), hasVertexNormals(true), hasUVs(false) {}

    Triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c,
             const Vec3 &na, const Vec3 &nb, const Vec3 &nc,
             const Vec2 &ta, const Vec2 &tb, const Vec2 &tc)
        : v0(a), v1(b), v2(c), n0(na), n1(nb), n2(nc),
          uv0(ta), uv1(tb), uv2(tc), hasVertexNormals(true), hasUVs(true) {}
};

struct Hit
{
    bool hit;
    float t;
    float u, v;
};

Hit intersectTriangle(const Ray &r, const Triangle &tri, float tMin = RAY_EPSILON, float tMax = 1e30f)
{
    Vec3 e1 = tri.v1 - tri.v0;
    Vec3 e2 = tri.v2 - tri.v0;
    Vec3 p = cross(r.dir, e2);
    float det = dot(e1, p);

    if (std::fabs(det) < 1e-8f)
        return {false, 0, 0, 0};
    float invDet = 1.0f / det;

    Vec3 T = r.origin - tri.v0;
    float u = dot(T, p) * invDet;
    if (u < 0.0f || u > 1.0f)
        return {false, 0, 0, 0};

    Vec3 q = cross(T, e1);
    float v = dot(r.dir, q) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return {false, 0, 0, 0};

    float t = dot(e2, q) * invDet;

    if (t < tMin || t > tMax)
        return {false, 0, 0, 0};

    return {true, t, u, v};
}

inline Vec3 faceNormal(const Triangle &tri)
{
    Vec3 e1 = tri.v1 - tri.v0;
    Vec3 e2 = tri.v2 - tri.v0;
    return normalize(cross(e1, e2));
}

extern bool g_forceFlatShading;

inline Vec3 interpolateNormal(const Triangle &tri, float u, float v)
{
    if (!tri.hasVertexNormals || g_forceFlatShading)
    {
        return faceNormal(tri);
    }
    float w = 1.0f - u - v;
    Vec3 normal = tri.n0 * w + tri.n1 * u + tri.n2 * v;
    return normalize(normal);
}

inline Vec2 interpolateUV(const Triangle &tri, float u, float v)
{
    if (!tri.hasUVs)
    {
        return Vec2(0, 0);
    }
    float w = 1.0f - u - v;
    return Vec2(
        tri.uv0.u * w + tri.uv1.u * u + tri.uv2.u * v,
        tri.uv0.v * w + tri.uv1.v * u + tri.uv2.v * v);
}

#endif
