#ifndef BVH_HPP
#define BVH_HPP

#include "primitives.hpp"
#include "math.hpp"
#include <vector>
#include <algorithm>
#include <limits>

struct AABB
{
    Vec3 min, max;

    AABB() : min(Vec3(1e30f, 1e30f, 1e30f)), max(Vec3(-1e30f, -1e30f, -1e30f)) {}

    AABB(const Vec3 &min, const Vec3 &max) : min(min), max(max) {}

    void expand(const Vec3 &p)
    {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    void expand(const AABB &other)
    {
        expand(other.min);
        expand(other.max);
    }

    float surfaceArea() const
    {
        Vec3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    bool intersect(const Ray &ray, float tMin, float tMax) const
    {
        for (int i = 0; i < 3; i++)
        {
            float origin = (i == 0 ? ray.origin.x : i == 1 ? ray.origin.y
                                                           : ray.origin.z);
            float dir = (i == 0 ? ray.dir.x : i == 1 ? ray.dir.y
                                                     : ray.dir.z);
            float bmin = (i == 0 ? min.x : i == 1 ? min.y
                                                  : min.z);
            float bmax = (i == 0 ? max.x : i == 1 ? max.y
                                                  : max.z);

            // Ray parallel to slab — no hit if origin outside
            if (std::fabs(dir) < 1e-8f)
            {
                if (origin < bmin || origin > bmax)
                    return false;
                continue;
            }

            float invD = 1.0f / dir;
            float t0 = (bmin - origin) * invD;
            float t1 = (bmax - origin) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;

            if (tMax < tMin)
                return false;
        }
        return true;
    }
};

struct BVHNode
{
    AABB bounds;
    int leftChild;
    int rightChild;
    int firstTriIdx;
    int triCount;

    BVHNode() : leftChild(-1), rightChild(-1), firstTriIdx(0), triCount(0) {}

    bool isLeaf() const { return leftChild == -1; }
};

class BVH
{
private:
    std::vector<BVHNode> nodes;
    std::vector<int> triIndices;
    int nodesUsed;

    static constexpr int MAX_TRIANGLES_PER_LEAF = 4;

    Vec3 getTriangleCentroid(const Triangle &tri) const
    {
        return Vec3(
            (tri.v0.x + tri.v1.x + tri.v2.x) / 3.0f,
            (tri.v0.y + tri.v1.y + tri.v2.y) / 3.0f,
            (tri.v0.z + tri.v1.z + tri.v2.z) / 3.0f);
    }

    float getCentroidAxis(const Vec3 &centroid, int axis) const
    {
        return axis == 0 ? centroid.x : axis == 1 ? centroid.y
                                                  : centroid.z;
    }

    int buildRecursive(const std::vector<Triangle> &triangles, int start, int end)
    {
        int nodeIdx = nodesUsed++;
        nodes[nodeIdx] = BVHNode();

        for (int i = start; i < end; i++)
        {
            const Triangle &tri = triangles[triIndices[i]];
            nodes[nodeIdx].bounds.expand(tri.v0);
            nodes[nodeIdx].bounds.expand(tri.v1);
            nodes[nodeIdx].bounds.expand(tri.v2);
        }

        int triCount = end - start;

        if (triCount <= MAX_TRIANGLES_PER_LEAF)
        {
            nodes[nodeIdx].firstTriIdx = start;
            nodes[nodeIdx].triCount = triCount;
            return nodeIdx;
        }

        Vec3 extent = nodes[nodeIdx].bounds.max - nodes[nodeIdx].bounds.min;
        int axis = 0;
        if (extent.y > extent.x)
            axis = 1;
        if (extent.z > (axis == 0 ? extent.x : extent.y))
            axis = 2;

        float splitPos;
        if (axis == 0)
            splitPos = nodes[nodeIdx].bounds.min.x + extent.x * 0.5f;
        else if (axis == 1)
            splitPos = nodes[nodeIdx].bounds.min.y + extent.y * 0.5f;
        else
            splitPos = nodes[nodeIdx].bounds.min.z + extent.z * 0.5f;

        int i = start;
        int j = end - 1;

        while (i <= j)
        {
            const Triangle &tri = triangles[triIndices[i]];
            Vec3 centroid = getTriangleCentroid(tri);
            float centroidAxis = getCentroidAxis(centroid, axis);

            if (centroidAxis < splitPos)
            {
                i++;
            }
            else
            {
                std::swap(triIndices[i], triIndices[j]);
                j--;
            }
        }

        int mid = i;

        // Fallback to median split if all triangles end up on one side
        if (mid == start || mid == end)
        {
            std::sort(triIndices.begin() + start, triIndices.begin() + end,
                      [&](int a, int b)
                      {
                          Vec3 centroidA = getTriangleCentroid(triangles[a]);
                          Vec3 centroidB = getTriangleCentroid(triangles[b]);
                          return getCentroidAxis(centroidA, axis) < getCentroidAxis(centroidB, axis);
                      });
            mid = start + triCount / 2;
        }

        nodes[nodeIdx].leftChild = buildRecursive(triangles, start, mid);
        nodes[nodeIdx].rightChild = buildRecursive(triangles, mid, end);

        return nodeIdx;
    }

public:
    BVH() : nodesUsed(0) {}

    void build(const std::vector<Triangle> &triangles)
    {
        if (triangles.empty())
            return;

        triIndices.resize(triangles.size());
        for (size_t i = 0; i < triangles.size(); i++)
        {
            triIndices[i] = i;
        }

        nodes.resize(triangles.size() * 2);
        nodesUsed = 0;

        buildRecursive(triangles, 0, triangles.size());

        nodes.resize(nodesUsed);
    }

    bool traverse(const Ray &ray, const std::vector<Triangle> &triangles,
                  Hit &bestHit, int &bestTriIndex, float &closestT,
                  uint64_t &intersectionTests) const
    {
        if (nodes.empty())
            return false;

        bool anyHit = false;

        // Stack-based traversal avoids recursion overhead
        int stack[64];
        int stackPtr = 0;
        stack[stackPtr++] = 0;

        while (stackPtr > 0)
        {
            int nodeIdx = stack[--stackPtr];
            const BVHNode &node = nodes[nodeIdx];

            if (!node.bounds.intersect(ray, RAY_EPSILON, closestT))
            {
                continue;
            }

            if (node.isLeaf())
            {
                for (int i = 0; i < node.triCount; i++)
                {
                    int triIdx = triIndices[node.firstTriIdx + i];
                    Hit hit = intersectTriangle(ray, triangles[triIdx]);
                    intersectionTests++;

                    if (hit.hit && hit.t < closestT)
                    {
                        closestT = hit.t;
                        bestHit = hit;
                        bestTriIndex = triIdx;
                        anyHit = true;
                    }
                }
            }
            else
            {
                stack[stackPtr++] = node.leftChild;
                stack[stackPtr++] = node.rightChild;
            }
        }

        return anyHit;
    }

    int getNodeCount() const { return nodesUsed; }

    int getDepth() const
    {
        if (nodes.empty())
            return 0;
        return getDepthRecursive(0);
    }

private:
    int getDepthRecursive(int nodeIdx) const
    {
        if (nodes[nodeIdx].isLeaf())
            return 1;

        int leftDepth = getDepthRecursive(nodes[nodeIdx].leftChild);
        int rightDepth = getDepthRecursive(nodes[nodeIdx].rightChild);
        return 1 + std::max(leftDepth, rightDepth);
    }
};

#endif
