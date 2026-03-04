#ifndef CAMERA_HPP
#define CAMERA_HPP
#include "helpers/math.hpp"
#include "helpers/primitives.hpp"

struct Camera
{
    Vec3 position;
    Vec3 target;
    Vec3 up;
    Mat4 viewMatrix;
    float fov;
    float aspect;
    int width, height;

    Camera(Vec3 pos, Vec3 tgt, float fovDeg, int w, int h)
        : position(pos), target(tgt), up(Vec3(0, 1, 0)),
          fov(fovDeg * 3.14159265f / 180.0f), width(w), height(h)
    {
        aspect = float(width) / float(height);
        updateViewMatrix();
    }

    void updateViewMatrix()
    {
        Vec3 forward = normalize(target - position);
        Vec3 right = normalize(cross(forward, up));
        Vec3 newUp = cross(right, forward);

        viewMatrix = Mat4::identity();
        viewMatrix.m[0][0] = right.x;
        viewMatrix.m[0][1] = right.y;
        viewMatrix.m[0][2] = right.z;
        viewMatrix.m[1][0] = newUp.x;
        viewMatrix.m[1][1] = newUp.y;
        viewMatrix.m[1][2] = newUp.z;
        viewMatrix.m[2][0] = -forward.x;
        viewMatrix.m[2][1] = -forward.y;
        viewMatrix.m[2][2] = -forward.z;

        Vec3 negPos = position * -1.0f;
        viewMatrix.m[0][3] = dot(right, negPos);
        viewMatrix.m[1][3] = dot(newUp, negPos);
        viewMatrix.m[2][3] = dot(forward, position);
    }

    Ray makeRay(float x, float y)
    {
        float halfTan = std::tan(fov * 0.5f);
        float u = (x + 0.5f) / float(width);
        float v = (y + 0.5f) / float(height);
        float sx = (2 * u - 1) * aspect * halfTan;
        float sy = (1 - 2 * v) * halfTan;

        Vec3 rayDir = normalize(Vec3(sx, sy, -1));

        Mat4 invView = viewMatrix.inverse();
        Vec3 worldOrigin = invView.transformPoint(Vec3(0, 0, 0));
        Vec3 worldDir = normalize(invView.transformDir(rayDir));

        return Ray{worldOrigin, worldDir};
    }
};

#endif
