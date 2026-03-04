#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include "../helpers/math.hpp"
#include "../helpers/primitives.hpp"
#include "../helpers/animation.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

struct Material
{
    Vec3 diffuseColor;
    Vec3 specularColor;
    float shininess;
    float ambient;
    float reflectivity; // 0.0 = no reflection, 1.0 = perfect mirror
    float glossiness;   // 0.0 = rough, 1.0 = perfect mirror (controls reflection blur)

    float transparency;       // 0.0 = opaque, 1.0 = fully transparent
    float ior;                // (1.0 = air, 1.33 = water, 1.5 = glass)
    Vec3 transmissionColor;  
    float absorptionDistance;

    std::string diffuseTexture;
    std::string normalTexture;
    std::string roughnessTexture;
    bool hasTexture;
    float textureScale;

    Material() : diffuseColor(1, 1, 1), specularColor(0.5, 0.5, 0.5),
                 shininess(16.0f), ambient(0.1f), reflectivity(0.0f), glossiness(1.0f),
                 transparency(0.0f), ior(1.5f), transmissionColor(1, 1, 1), absorptionDistance(1.0f),
                 diffuseTexture(""), normalTexture(""), roughnessTexture(""), hasTexture(false),
                 textureScale(1.0f) {}
};

class Object
{
protected:
    std::string name;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Mat4 modelMatrix;
    bool isDirty;
    Animation *animation = nullptr;
    float animationTime = 0.0f;

    void updateModelMatrix()
    {
        Mat4 S = Mat4::scale(scale);
        Mat4 Rx = Mat4::rotationX(rotation.x);
        Mat4 Ry = Mat4::rotationY(rotation.y);
        Mat4 Rz = Mat4::rotationZ(rotation.z);
        Mat4 T = Mat4::translation(position);

        modelMatrix = T * Ry * Rx * Rz * S;
        isDirty = false;
    }

    Material material;

public:
    Object()
        : position(Vec3(0, 0, 0)),
          rotation(Vec3(0, 0, 0)),
          scale(Vec3(1, 1, 1)),
          isDirty(true)
    {
        modelMatrix = Mat4::identity();
    }

    virtual ~Object() {}

    const Material &getMaterial() const { return material; }
    const Vec3 &getPosition() const { return position; }
    const Vec3 &getRotation() const { return rotation; }
    const Vec3 &getScale() const { return scale; }
    const Mat4 &getModelMatrix()
    {
        if (isDirty)
            updateModelMatrix();
        return modelMatrix;
    }

    const std::string &getName() const { return name; }
    void setName(const std::string &n) { name = n; }

    void setPosition(const Vec3 &pos)
    {
        position = pos;
        isDirty = true;
    }

    void setRotation(const Vec3 &rot)
    {
        rotation = rot;
        isDirty = true;
    }

    void setScale(const Vec3 &s)
    {
        scale = s;
        isDirty = true;
    }

    void setTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &s)
    {
        position = pos;
        rotation = rot;
        scale = s;
        isDirty = true;
    }

    void translate(const Vec3 &delta)
    {
        position = position + delta;
        isDirty = true;
    }

    void rotate(const Vec3 &delta)
    {
        rotation = rotation + delta;
        isDirty = true;
    }

    void scaleBy(const Vec3 &factor)
    {
        scale = Vec3(scale.x * factor.x, scale.y * factor.y, scale.z * factor.z);
        isDirty = true;
    }

    void setMaterial(const Material &mat) { material = mat; }

    Mat4 getInverseMatrix()
    {
        if (isDirty)
            updateModelMatrix();
        return modelMatrix.inverse();
    }

    void setAnimation(Animation *anim)
    {
        animation = anim;
        animationTime = 0.0f;
    }

    void updateAnimation(float deltaTime)
    {
        if (!animation)
            return;

        animationTime += deltaTime;
        Keyframe kf = animation->interpolate(animationTime);

        setTransform(kf.position, kf.rotation, kf.scale);
    }

    float getAnimationTime() const { return animationTime; }

    virtual std::vector<Triangle> getWorldTriangles() = 0;
};

#endif
