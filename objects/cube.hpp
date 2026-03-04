#ifndef CUBE_HPP
#define CUBE_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include "../helpers/math.hpp"
#include <algorithm>
#include <vector>
#include <cmath>
#include "object.hpp"

class Cube : public Object
{
private:
    std::vector<Triangle> localTris;

public:
    Cube() : Object()
    {
        Vec3 localVerts[8] = {
            Vec3(-0.5f, -0.5f, -0.5f),
            Vec3(0.5f, -0.5f, -0.5f),
            Vec3(0.5f, 0.5f, -0.5f),
            Vec3(-0.5f, 0.5f, -0.5f),
            Vec3(-0.5f, -0.5f, 0.5f),
            Vec3(0.5f, -0.5f, 0.5f),
            Vec3(0.5f, 0.5f, 0.5f),
            Vec3(-0.5f, 0.5f, 0.5f)};

        const int faces[6][4] = {
            {0, 1, 2, 3}, 
            {5, 4, 7, 6},
            {4, 0, 3, 7}, 
            {1, 5, 6, 2}, 
            {3, 2, 6, 7},
            {4, 5, 1, 0} 
        };

        for (int f = 0; f < 6; f++)
        {
            int a = faces[f][0];
            int b = faces[f][1];
            int c = faces[f][2];
            int d = faces[f][3];
            localTris.push_back({localVerts[a], localVerts[b], localVerts[c]});
            localTris.push_back({localVerts[a], localVerts[c], localVerts[d]});
        }
    }

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
