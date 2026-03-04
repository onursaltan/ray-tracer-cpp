#ifndef PLANE_HPP
#define PLANE_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include "../helpers/math.hpp"
#include "../helpers/primitives.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include "object.hpp"

class Plane : public Object
{
private:
    float size;
    std::vector<Triangle> localTris;

public:
    Plane(float s = 10.0f) : Object(), size(s)
    {
        generateMesh();
    }

    void generateMesh()
    {
        localTris.clear();

        float half = size / 2.0f;

        Vec3 v0(-half, 0, -half);
        Vec3 v1(half, 0, -half);
        Vec3 v2(half, 0, half);
        Vec3 v3(-half, 0, half);

        localTris.push_back({v0, v1, v2});
        localTris.push_back({v0, v2, v3});
    }

    void setSize(float s)
    {
        size = s;
        generateMesh();
    }

    float getSize() const { return size; }

    std::vector<Triangle> getWorldTriangles() override
    {
        if (isDirty)
            updateModelMatrix();

        std::vector<Triangle> worldTris;
        for (const auto &tri : localTris)
        {
            Triangle worldTri;
            worldTri.v0 = modelMatrix.transformPoint(tri.v0);
            worldTri.v1 = modelMatrix.transformPoint(tri.v1);
            worldTri.v2 = modelMatrix.transformPoint(tri.v2);
            worldTris.push_back(worldTri);
        }
        return worldTris;
    }
};

#endif
