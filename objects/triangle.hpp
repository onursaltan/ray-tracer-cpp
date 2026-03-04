#include <cstdint>
#include <fstream>
#include <iostream>
#include "../helpers/math.hpp"
#include "../helpers/primitives.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include "object.hpp"

class TriangleObject : public Object
{
private:
    Vec3 localV0, localV1, localV2;

public:
    TriangleObject(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
        : Object(), localV0(v0), localV1(v1), localV2(v2)
    {
    }

    TriangleObject()
        : Object(),
          localV0(Vec3(-0.5f, -0.5f, 0.0f)),
          localV1(Vec3(0.5f, -0.5f, 0.0f)),
          localV2(Vec3(0.0f, 0.5f, 0.0f))
    {
    }

    void setVertices(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
    {
        localV0 = v0;
        localV1 = v1;
        localV2 = v2;
    }

    void getLocalVertices(Vec3 &v0, Vec3 &v1, Vec3 &v2) const
    {
        v0 = localV0;
        v1 = localV1;
        v2 = localV2;
    }

    std::vector<Triangle> getWorldTriangles() override
    {
        if (isDirty)
            updateModelMatrix();

        std::vector<Triangle> triangles;
        Triangle worldTri;
        worldTri.v0 = modelMatrix.transformPoint(localV0);
        worldTri.v1 = modelMatrix.transformPoint(localV1);
        worldTri.v2 = modelMatrix.transformPoint(localV2);
        triangles.push_back(worldTri);

        return triangles;
    }

    Vec3 getWorldNormal()
    {
        if (isDirty)
            updateModelMatrix();

        Vec3 worldV0 = modelMatrix.transformPoint(localV0);
        Vec3 worldV1 = modelMatrix.transformPoint(localV1);
        Vec3 worldV2 = modelMatrix.transformPoint(localV2);

        Vec3 e1 = worldV1 - worldV0;
        Vec3 e2 = worldV2 - worldV0;
        return normalize(cross(e1, e2));
    }
};
