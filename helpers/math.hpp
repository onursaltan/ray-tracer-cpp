#ifndef MATH_HPP
#define MATH_HPP

#include <iostream>
#include <cmath>

struct Vec3
{
    float x, y, z;

    Vec3(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3 &b) const { return Vec3(x + b.x, y + b.y, z + b.z); }
    Vec3 operator-(const Vec3 &b) const { return Vec3(x - b.x, y - b.y, z - b.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
};

float dot(const Vec3 &a, const Vec3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

float length(const Vec3 &v)
{
    return std::sqrt(dot(v, v));
}

Vec3 normalize(const Vec3 &v)
{
    float L = length(v);
    if (L == 0)
        return v;
    return v * (1.0f / L);
}

struct Mat4
{
    float m[4][4];

    Mat4()
    {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    static Mat4 identity()
    {
        return Mat4();
    }

    static Mat4 translation(const Vec3 &t)
    {
        Mat4 mat;
        mat.m[0][3] = t.x;
        mat.m[1][3] = t.y;
        mat.m[2][3] = t.z;
        return mat;
    }

    static Mat4 scale(const Vec3 &s)
    {
        Mat4 mat;
        mat.m[0][0] = s.x;
        mat.m[1][1] = s.y;
        mat.m[2][2] = s.z;
        return mat;
    }

    static Mat4 rotationX(float angle)
    {
        Mat4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[1][1] = c;
        mat.m[1][2] = -s;
        mat.m[2][1] = s;
        mat.m[2][2] = c;
        return mat;
    }

    static Mat4 rotationY(float angle)
    {
        Mat4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][2] = s;
        mat.m[2][0] = -s;
        mat.m[2][2] = c;
        return mat;
    }

    static Mat4 rotationZ(float angle)
    {
        Mat4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][1] = -s;
        mat.m[1][0] = s;
        mat.m[1][1] = c;
        return mat;
    }

    Mat4 operator*(const Mat4 &other) const
    {
        Mat4 result;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++)
                {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }

    Vec3 transformPoint(const Vec3 &v) const
    {
        float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
        float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
        float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
        float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];
        return Vec3(x / w, y / w, z / w);
    }

    Vec3 transformDir(const Vec3 &v) const
    {
        float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
        float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
        float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
        return Vec3(x, y, z);
    }

    Mat4 inverse() const
    {
        Mat4 inv;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                inv.m[i][j] = m[j][i];

        Vec3 t(m[0][3], m[1][3], m[2][3]);
        Vec3 invT = inv.transformDir(t * -1.0f);
        inv.m[0][3] = invT.x;
        inv.m[1][3] = invT.y;
        inv.m[2][3] = invT.z;

        return inv;
    }
};

inline Vec3 refract(const Vec3 &incident, const Vec3 &normal, float eta)
{
    float cosi = -dot(incident, normal);
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    if (k < 0.0f)
    {
        return Vec3(0.0f, 0.0f, 0.0f);
    }
    return incident * eta + normal * (eta * cosi - std::sqrt(k));
}

inline float fresnelSchlick(float cosTheta, float ior)
{
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * std::pow(1.0f - cosTheta, 5.0f);
}

inline Vec3 beerLambertAbsorption(const Vec3 &transmissionColor, float distance, float absorptionDistance)
{
    Vec3 absorption;
    absorption.x = transmissionColor.x > 0.0f ? -std::log(transmissionColor.x) / absorptionDistance : 10.0f;
    absorption.y = transmissionColor.y > 0.0f ? -std::log(transmissionColor.y) / absorptionDistance : 10.0f;
    absorption.z = transmissionColor.z > 0.0f ? -std::log(transmissionColor.z) / absorptionDistance : 10.0f;

    // I = I0 * e^(-a * d)
    Vec3 attenuation;
    attenuation.x = std::exp(-absorption.x * distance);
    attenuation.y = std::exp(-absorption.y * distance);
    attenuation.z = std::exp(-absorption.z * distance);
    return attenuation;
}

#endif // MATH_HPP
