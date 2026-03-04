#ifndef LOADER_HPP
#define LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include "math.hpp"
#include "primitives.hpp"
#include "material-registry.hpp"
#include "animation.hpp"
#include "../camera.hpp"
#include "../objects/object.hpp"
#include "../objects/cube.hpp"
#include "../objects/sphere.hpp"
#include "../objects/plane.hpp"
#include "../lights.hpp"

bool loadScene(const std::string &filename,
               Camera *&camera,
               std::vector<Object *> &objects,
               std::vector<PointLight> &pointLights,
               std::vector<DirectionalLight> &directionalLights,
               std::vector<SpotLight> &spotLights,
               std::map<std::string, Animation *> &animations,
               int width, int height)
{
    std::ifstream file(filename);
    if (!file)
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "camera")
        {
            Vec3 pos, target;
            float fov;
            iss >> pos.x >> pos.y >> pos.z;
            iss >> target.x >> target.y >> target.z;
            iss >> fov;

            camera = new Camera(pos, target, fov, width, height);
        }
        else if (type == "pointlight")
        {
            PointLight light;
            iss >> light.position.x >> light.position.y >> light.position.z;
            iss >> light.color.x >> light.color.y >> light.color.z;
            iss >> light.intensity;
            iss >> light.radius;
            pointLights.push_back(light);
        }
        else if (type == "directionallight")
        {
            DirectionalLight light;
            iss >> light.direction.x >> light.direction.y >> light.direction.z;
            light.direction = normalize(light.direction);
            iss >> light.color.x >> light.color.y >> light.color.z;
            iss >> light.intensity;
            directionalLights.push_back(light);
        }
        else if (type == "spotlight")
        {
            std::string name;
            SpotLight light;
            iss >> name;
            iss >> light.position.x >> light.position.y >> light.position.z;
            iss >> light.direction.x >> light.direction.y >> light.direction.z;
            light.direction = normalize(light.direction);
            iss >> light.color.x >> light.color.y >> light.color.z;
            iss >> light.intensity;
            iss >> light.innerAngle >> light.outerAngle;
            iss >> light.radius;
            spotLights.push_back(light);
        }
        else if (type == "cube")
        {
            std::string name, materialName;
            Vec3 pos, rot, scl;
            iss >> name;
            iss >> pos.x >> pos.y >> pos.z;
            iss >> rot.x >> rot.y >> rot.z;
            iss >> scl.x >> scl.y >> scl.z;
            iss >> materialName;

            Cube *cube = new Cube();
            cube->setName(name);
            cube->setTransform(pos, rot, scl);
            cube->setMaterial(MaterialRegistry::getMaterial(materialName));
            objects.push_back(cube);
        }
        else if (type == "sphere")
        {
            std::string name, materialName;
            Vec3 pos, rot, scl;
            float radius;
            int segments;
            iss >> name;
            iss >> pos.x >> pos.y >> pos.z;
            iss >> rot.x >> rot.y >> rot.z;
            iss >> scl.x >> scl.y >> scl.z;
            iss >> radius >> segments;
            iss >> materialName;

            Sphere *sphere = new Sphere(radius, segments);
            sphere->setName(name);
            sphere->setTransform(pos, rot, scl);
            sphere->setMaterial(MaterialRegistry::getMaterial(materialName));
            objects.push_back(sphere);
        }
        else if (type == "plane")
        {
            std::string name, materialName;
            Vec3 pos, rot, scl;
            float size;
            iss >> name;
            iss >> pos.x >> pos.y >> pos.z;
            iss >> rot.x >> rot.y >> rot.z;
            iss >> scl.x >> scl.y >> scl.z;
            iss >> size;
            iss >> materialName;

            Plane *plane = new Plane(size);
            plane->setName(name);
            plane->setTransform(pos, rot, scl);
            plane->setMaterial(MaterialRegistry::getMaterial(materialName));
            objects.push_back(plane);
        }
        else if (type == "animation")
        {
            std::string name, objectName, animType;
            float duration;
            bool looping;
            iss >> name >> objectName >> animType >> duration >> looping;

            Animation *anim = new Animation();
            anim->setLooping(looping);

            if (animType == "rotate")
            {
                Vec3 startRot, endRot;
                float startX, startY, startZ, endX, endY, endZ;
                iss >> startX >> startY >> startZ >> endX >> endY >> endZ;
                startRot = Vec3(startX, startY, startZ);
                endRot = Vec3(endX, endY, endZ);

                anim->addKeyframe({0.0f, Vec3(0, 0, 0), startRot, Vec3(1, 1, 1)});
                anim->addKeyframe({duration, Vec3(0, 0, 0), endRot, Vec3(1, 1, 1)});
            }
            else if (animType == "move")
            {
                Vec3 startPos, endPos;
                float startX, startY, startZ, endX, endY, endZ;
                iss >> startX >> startY >> startZ >> endX >> endY >> endZ;
                startPos = Vec3(startX, startY, startZ);
                endPos = Vec3(endX, endY, endZ);

                anim->addKeyframe({0.0f, startPos, Vec3(0, 0, 0), Vec3(1, 1, 1)});
                anim->addKeyframe({duration, endPos, Vec3(0, 0, 0), Vec3(1, 1, 1)});
            }
            else if (animType == "moveAround")
            {
                Vec3 center;
                float radius;
                float startX, startY, startZ;
                iss >> startX >> startY >> startZ >> radius;
                center = Vec3(startX, startY, startZ);

                // Circular path with 4 keyframes at cardinal points
                float quarterDuration = duration / 4.0f;
                anim->addKeyframe({0.0f, center + Vec3(radius, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
                anim->addKeyframe({quarterDuration, center + Vec3(0, 0, radius), Vec3(0, 0, 0), Vec3(1, 1, 1)});
                anim->addKeyframe({2 * quarterDuration, center + Vec3(-radius, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
                anim->addKeyframe({3 * quarterDuration, center + Vec3(0, 0, -radius), Vec3(0, 0, 0), Vec3(1, 1, 1)});
                anim->addKeyframe({duration, center + Vec3(radius, 0, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)});
            }

            animations[name] = anim;

            for (auto *obj : objects)
            {
                if (obj->getName() == objectName)
                {
                    obj->setAnimation(anim);
                    std::cerr << "Assigned animation '" << name << "' to object '" << objectName << "'\n";
                    break;
                }
            }
        }
    }
    std::cerr << "Scene loaded \n";

    return true;
}

#endif
