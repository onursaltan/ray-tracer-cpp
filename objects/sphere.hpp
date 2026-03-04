#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include "../helpers/math.hpp"
#include "../helpers/primitives.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include "object.hpp"

class Sphere : public Object
{
private:
    float radius;
    int segments;

    void generateMesh()
    {
        localTris.clear();

        for (int lat = 0; lat < segments; lat++)
        {
            float theta1 = lat * 3.14159265f / segments;
            float theta2 = (lat + 1) * 3.14159265f / segments;

            for (int lon = 0; lon < segments * 2; lon++)
            {
                float phi1 = lon * 3.14159265f / segments;
                float phi2 = (lon + 1) * 3.14159265f / segments;

                Vec3 v1(
                    radius * std::sin(theta1) * std::cos(phi1),
                    radius * std::cos(theta1),
                    radius * std::sin(theta1) * std::sin(phi1));

                Vec3 v2(
                    radius * std::sin(theta1) * std::cos(phi2),
                    radius * std::cos(theta1),
                    radius * std::sin(theta1) * std::sin(phi2));

                Vec3 v3(
                    radius * std::sin(theta2) * std::cos(phi2),
                    radius * std::cos(theta2),
                    radius * std::sin(theta2) * std::sin(phi2));

                Vec3 v4(
                    radius * std::sin(theta2) * std::cos(phi1),
                    radius * std::cos(theta2),
                    radius * std::sin(theta2) * std::sin(phi1));

                Vec3 n1 = normalize(v1);
                Vec3 n2 = normalize(v2);
                Vec3 n3 = normalize(v3);
                Vec3 n4 = normalize(v4);

                float u1 = (float)lon / (segments * 2);
                float u2 = (float)(lon + 1) / (segments * 2);
                float v1_uv = (float)lat / segments;
                float v2_uv = (float)(lat + 1) / segments;

                if (lat != 0)
                {
                    Triangle tri;
                    tri.v0 = v1;
                    tri.v1 = v2;
                    tri.v2 = v3;
                    tri.n0 = n1;
                    tri.n1 = n2;
                    tri.n2 = n3;
                    tri.uv0 = Vec2(u1, v1_uv);
                    tri.uv1 = Vec2(u2, v1_uv);
                    tri.uv2 = Vec2(u2, v2_uv);
                    tri.hasVertexNormals = true;
                    tri.hasUVs = true;
                    localTris.push_back(tri);
                }
                if (lat != segments - 1)
                {
                    Triangle tri;
                    tri.v0 = v1;
                    tri.v1 = v3;
                    tri.v2 = v4;
                    tri.n0 = n1;
                    tri.n1 = n3;
                    tri.n2 = n4;
                    tri.uv0 = Vec2(u1, v1_uv);
                    tri.uv1 = Vec2(u2, v2_uv);
                    tri.uv2 = Vec2(u1, v2_uv);
                    tri.hasVertexNormals = true;
                    tri.hasUVs = true;
                    localTris.push_back(tri);
                }
            }
        }
    }

    std::vector<Triangle> localTris;

public:
    Sphere(float r = 1.0f, int segs = 16) : Object(), radius(r), segments(segs)
    {
        generateMesh();
    }

    void setRadius(float r)
    {
        radius = r;
        generateMesh();
    }

    void setSegments(int segs)
    {
        segments = segs;
        generateMesh();
    }

    float getRadius() const { return radius; }

    std::vector<Triangle> getWorldTriangles() override
    {
        if (isDirty)
            updateModelMatrix();

        std::vector<Triangle> worldTris;
        worldTris.reserve(localTris.size());

        bool uniformScale = (std::fabs(scale.x - scale.y) < 1e-6f &&
                             std::fabs(scale.y - scale.z) < 1e-6f);

        for (const auto &tri : localTris)
        {
            Triangle worldTri;

            worldTri.v0 = modelMatrix.transformPoint(tri.v0);
            worldTri.v1 = modelMatrix.transformPoint(tri.v1);
            worldTri.v2 = modelMatrix.transformPoint(tri.v2);

            if (tri.hasVertexNormals)
            {
                if (uniformScale)
                {
                    worldTri.n0 = normalize(modelMatrix.transformDir(tri.n0));
                    worldTri.n1 = normalize(modelMatrix.transformDir(tri.n1));
                    worldTri.n2 = normalize(modelMatrix.transformDir(tri.n2));
                }
                else
                {
                    // For non-uniform scale, use inverse transpose
                    Mat4 invMat = modelMatrix.inverse();
                    worldTri.n0 = normalize(Vec3(
                        invMat.m[0][0] * tri.n0.x + invMat.m[1][0] * tri.n0.y + invMat.m[2][0] * tri.n0.z,
                        invMat.m[0][1] * tri.n0.x + invMat.m[1][1] * tri.n0.y + invMat.m[2][1] * tri.n0.z,
                        invMat.m[0][2] * tri.n0.x + invMat.m[1][2] * tri.n0.y + invMat.m[2][2] * tri.n0.z));
                    worldTri.n1 = normalize(Vec3(
                        invMat.m[0][0] * tri.n1.x + invMat.m[1][0] * tri.n1.y + invMat.m[2][0] * tri.n1.z,
                        invMat.m[0][1] * tri.n1.x + invMat.m[1][1] * tri.n1.y + invMat.m[2][1] * tri.n1.z,
                        invMat.m[0][2] * tri.n1.x + invMat.m[1][2] * tri.n1.y + invMat.m[2][2] * tri.n1.z));
                    worldTri.n2 = normalize(Vec3(
                        invMat.m[0][0] * tri.n2.x + invMat.m[1][0] * tri.n2.y + invMat.m[2][0] * tri.n2.z,
                        invMat.m[0][1] * tri.n2.x + invMat.m[1][1] * tri.n2.y + invMat.m[2][1] * tri.n2.z,
                        invMat.m[0][2] * tri.n2.x + invMat.m[1][2] * tri.n2.y + invMat.m[2][2] * tri.n2.z));
                }
                worldTri.hasVertexNormals = true;
            }

            worldTri.uv0 = tri.uv0;
            worldTri.uv1 = tri.uv1;
            worldTri.uv2 = tri.uv2;
            worldTri.hasUVs = tri.hasUVs;

            worldTris.push_back(worldTri);
        }
        return worldTris;
    }
};

#endif
