#ifndef JSON_LOADER_HPP
#define JSON_LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
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
#include "../objects/mesh.hpp"
#include "../lights.hpp"

std::vector<float> parseArray(const std::string &str)
{
    std::vector<float> values;
    size_t start = str.find('[');
    size_t end = str.find(']');

    if (start == std::string::npos || end == std::string::npos)
        return values;

    std::string content = str.substr(start + 1, end - start - 1);
    std::istringstream iss(content);
    float value;
    char comma;

    while (iss >> value)
    {
        values.push_back(value);
        iss >> comma;
    }

    return values;
}

float parseFloat(const std::string &str)
{
    size_t colonPos = str.find(':');
    if (colonPos == std::string::npos)
        return 0.0f;

    std::string value = str.substr(colonPos + 1);
    size_t commaPos = value.find(',');
    if (commaPos != std::string::npos)
        value = value.substr(0, commaPos);

    return std::stof(value);
}

int parseInt(const std::string &str)
{
    size_t colonPos = str.find(':');
    if (colonPos == std::string::npos)
        return 0;

    std::string value = str.substr(colonPos + 1);
    size_t commaPos = value.find(',');
    if (commaPos != std::string::npos)
        value = value.substr(0, commaPos);

    return std::stoi(value);
}

std::string parseString(const std::string &str)
{
    size_t colonPos = str.find(':');
    if (colonPos == std::string::npos)
        return "";

    size_t firstQuote = str.find('\"', colonPos);
    if (firstQuote == std::string::npos)
        return "";

    size_t lastQuote = str.find('\"', firstQuote + 1);
    if (lastQuote == std::string::npos)
        return "";

    return str.substr(firstQuote + 1, lastQuote - firstQuote - 1);
}

bool loadSceneJSON(const std::string &filename,
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
    std::string currentSection;
    std::string currentObjectType;

    std::string objName, objMaterial, objFilePath;
    Vec3 objPos, objRot, objScale(1, 1, 1);
    float objRadius = 0, objSize = 0;
    int objSegments = 32;

    struct AnimData {
        std::string name, object, type;
        float duration = 1.0f;
        bool looping = false;
        Vec3 startPos, endPos;
        Vec3 startRot, endRot;
        Vec3 startScale{1,1,1}, endScale{1,1,1};
        Vec3 center;
        float radius = 3.0f;
    };
    AnimData animData;
    bool buildingAnim = false;

    while (std::getline(file, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty())
            continue;

        if (line.find("\"camera\"") != std::string::npos)
        {
            currentSection = "camera";
            continue;
        }
        else if (line.find("\"lights\"") != std::string::npos)
        {
            currentSection = "lights";
            continue;
        }
        else if (line.find("\"objects\"") != std::string::npos)
        {
            currentSection = "objects";
            continue;
        }
        else if (line.find("\"pointLights\"") != std::string::npos)
        {
            currentObjectType = "pointLights";
            continue;
        }
        else if (line.find("\"directionalLights\"") != std::string::npos)
        {
            currentObjectType = "directionalLights";
            continue;
        }
        else if (line.find("\"spotLights\"") != std::string::npos)
        {
            currentObjectType = "spotLights";
            continue;
        }
        else if (line.find("\"spheres\"") != std::string::npos)
        {
            currentObjectType = "spheres";
            continue;
        }
        else if (line.find("\"cubes\"") != std::string::npos)
        {
            currentObjectType = "cubes";
            continue;
        }
        else if (line.find("\"planes\"") != std::string::npos)
        {
            currentObjectType = "planes";
            continue;
        }
        else if (line.find("\"models\"") != std::string::npos)
        {
            currentObjectType = "models";
            continue;
        }
        else if (line.find("\"animations\"") != std::string::npos && line.find("\"enabled\"") == std::string::npos)
        {
            currentSection = "animations";
            continue;
        }

        if (currentSection == "camera")
        {
            if (line.find("\"position\"") != std::string::npos)
            {
                auto vals = parseArray(line);
                if (vals.size() >= 3)
                {
                    Vec3 pos(vals[0], vals[1], vals[2]);
                    objPos = pos;
                }
            }
            else if (line.find("\"target\"") != std::string::npos)
            {
                auto vals = parseArray(line);
                if (vals.size() >= 3)
                {
                    Vec3 target(vals[0], vals[1], vals[2]);
                    float fov = 60.0f;
                    if (camera == nullptr && objPos.x != 0 || objPos.y != 0 || objPos.z != 0)
                    {
                        // Store target in objRot temporarily until fov is parsed
                        objRot = target;
                    }
                }
            }
            else if (line.find("\"fov\"") != std::string::npos)
            {
                float fov = parseFloat(line);
                if (camera == nullptr)
                {
                    camera = new Camera(objPos, objRot, fov, width, height);
                }
            }
        }

        if (currentSection == "lights" && currentObjectType == "pointLights")
        {
            static PointLight tempLight;
            static bool buildingLight = false;

            if (line.find("{") != std::string::npos && line.find("\"pointLights\"") == std::string::npos)
            {
                buildingLight = true;
                tempLight = PointLight();
            }
            else if (buildingLight)
            {
                if (line.find("\"position\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempLight.position = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"color\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempLight.color = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"intensity\"") != std::string::npos)
                {
                    tempLight.intensity = parseFloat(line);
                }
                else if (line.find("\"radius\"") != std::string::npos)
                {
                    tempLight.radius = parseFloat(line);
                }
                else if (line.find("}") != std::string::npos)
                {
                    pointLights.push_back(tempLight);
                    buildingLight = false;
                }
            }
        }

        if (currentSection == "lights" && currentObjectType == "directionalLights")
        {
            static DirectionalLight tempDirLight;
            static bool buildingDirLight = false;

            if (line.find("{") != std::string::npos && line.find("\"directionalLights\"") == std::string::npos)
            {
                buildingDirLight = true;
                tempDirLight = DirectionalLight();
            }
            else if (buildingDirLight)
            {
                if (line.find("\"direction\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempDirLight.direction = normalize(Vec3(vals[0], vals[1], vals[2]));
                }
                else if (line.find("\"color\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempDirLight.color = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"intensity\"") != std::string::npos)
                {
                    tempDirLight.intensity = parseFloat(line);
                }
                else if (line.find("}") != std::string::npos)
                {
                    directionalLights.push_back(tempDirLight);
                    buildingDirLight = false;
                }
            }
        }

        if (currentSection == "lights" && currentObjectType == "spotLights")
        {
            static SpotLight tempSpotLight;
            static bool buildingSpotLight = false;

            if (line.find("{") != std::string::npos && line.find("\"spotLights\"") == std::string::npos)
            {
                buildingSpotLight = true;
                tempSpotLight = SpotLight();
            }
            else if (buildingSpotLight)
            {
                if (line.find("\"position\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempSpotLight.position = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"direction\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempSpotLight.direction = normalize(Vec3(vals[0], vals[1], vals[2]));
                }
                else if (line.find("\"color\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        tempSpotLight.color = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"intensity\"") != std::string::npos)
                {
                    tempSpotLight.intensity = parseFloat(line);
                }
                else if (line.find("\"innerAngle\"") != std::string::npos)
                {
                    tempSpotLight.innerAngle = parseFloat(line) * 3.14159265359f / 180.0f;
                }
                else if (line.find("\"outerAngle\"") != std::string::npos)
                {
                    tempSpotLight.outerAngle = parseFloat(line) * 3.14159265359f / 180.0f;
                }
                else if (line.find("\"radius\"") != std::string::npos)
                {
                    tempSpotLight.radius = parseFloat(line);
                }
                else if (line.find("}") != std::string::npos)
                {
                    spotLights.push_back(tempSpotLight);
                    buildingSpotLight = false;
                }
            }
        }

        if (currentSection == "objects")
        {
            static bool buildingObject = false;

            if (line.find("{") != std::string::npos &&
                line.find("\"spheres\"") == std::string::npos &&
                line.find("\"cubes\"") == std::string::npos &&
                line.find("\"planes\"") == std::string::npos &&
                line.find("\"models\"") == std::string::npos &&
                line.find("\"objects\"") == std::string::npos)
            {
                buildingObject = true;
                objName = "";
                objMaterial = "white";
                objFilePath = "";
                objPos = Vec3(0, 0, 0);
                objRot = Vec3(0, 0, 0);
                objScale = Vec3(1, 1, 1);
                objRadius = 1.0f;
                objSize = 10.0f;
                objSegments = 32;
            }
            else if (buildingObject)
            {
                if (line.find("\"name\"") != std::string::npos)
                {
                    objName = parseString(line);
                }
                else if (line.find("\"position\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        objPos = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"rotation\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                    {
                        const float DEG_TO_RAD = 3.14159265359f / 180.0f;
                        objRot = Vec3(vals[0] * DEG_TO_RAD, vals[1] * DEG_TO_RAD, vals[2] * DEG_TO_RAD);
                    }
                }
                else if (line.find("\"scale\"") != std::string::npos)
                {
                    auto vals = parseArray(line);
                    if (vals.size() >= 3)
                        objScale = Vec3(vals[0], vals[1], vals[2]);
                }
                else if (line.find("\"radius\"") != std::string::npos)
                {
                    objRadius = parseFloat(line);
                }
                else if (line.find("\"size\"") != std::string::npos)
                {
                    objSize = parseFloat(line);
                }
                else if (line.find("\"segments\"") != std::string::npos)
                {
                    objSegments = parseInt(line);
                }
                else if (line.find("\"material\"") != std::string::npos)
                {
                    objMaterial = parseString(line);
                }
                else if (line.find("\"filepath\"") != std::string::npos || line.find("\"file\"") != std::string::npos)
                {
                    objFilePath = parseString(line);
                }
                else if (line.find("}") != std::string::npos)
                {
                    if (currentObjectType == "spheres")
                    {
                        Sphere *sphere = new Sphere(objRadius, objSegments);
                        sphere->setName(objName);
                        sphere->setTransform(objPos, objRot, objScale);
                        sphere->setMaterial(MaterialRegistry::getMaterial(objMaterial));
                        objects.push_back(sphere);
                    }
                    else if (currentObjectType == "cubes")
                    {
                        Cube *cube = new Cube();
                        cube->setName(objName);
                        cube->setTransform(objPos, objRot, objScale);
                        cube->setMaterial(MaterialRegistry::getMaterial(objMaterial));
                        objects.push_back(cube);
                    }
                    else if (currentObjectType == "planes")
                    {
                        Plane *plane = new Plane(objSize);
                        plane->setName(objName);
                        plane->setTransform(objPos, objRot, objScale);
                        plane->setMaterial(MaterialRegistry::getMaterial(objMaterial));
                        objects.push_back(plane);
                    }
                    else if (currentObjectType == "models")
                    {
                        Mesh *mesh = new Mesh(objFilePath);
                        if (mesh->isLoaded())
                        {
                            if (mesh->hasMtlMaterials())
                            {
                                MaterialRegistry::registerMaterialsFromMTL(mesh->getMtlMaterials());
                            }

                            mesh->setName(objName);
                            mesh->setTransform(objPos, objRot, objScale);

                            if (!objMaterial.empty())
                            {
                                mesh->setMaterial(MaterialRegistry::getMaterial(objMaterial));
                            }
                            else if (mesh->hasMtlMaterials())
                            {
                                std::string firstMat = mesh->getMtlMaterials().begin()->first;
                                mesh->setMaterial(MaterialRegistry::getMaterial(firstMat));
                            }
                            else
                            {
                                mesh->setMaterial(MaterialRegistry::getMaterial("white"));
                            }

                            objects.push_back(mesh);
                        }
                        else
                        {
                            std::cerr << "Warning: Failed to load model: " << objFilePath << "\n";
                            delete mesh;
                        }
                    }

                    buildingObject = false;
                }
            }
        }

        if (currentSection == "animations")
        {
            if (line.find("{") != std::string::npos && line.find("\"animations\"") == std::string::npos)
            {
                buildingAnim = true;
                animData = AnimData();
            }
            else if (buildingAnim)
            {
                if (line.find("\"name\"") != std::string::npos)
                    animData.name = parseString(line);
                else if (line.find("\"object\"") != std::string::npos)
                    animData.object = parseString(line);
                else if (line.find("\"type\"") != std::string::npos)
                    animData.type = parseString(line);
                else if (line.find("\"duration\"") != std::string::npos)
                    animData.duration = parseFloat(line);
                else if (line.find("\"looping\"") != std::string::npos)
                    animData.looping = (line.find("true") != std::string::npos);
                else if (line.find("\"startPosition\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3) animData.startPos = Vec3(v[0], v[1], v[2]);
                }
                else if (line.find("\"endPosition\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3) animData.endPos = Vec3(v[0], v[1], v[2]);
                }
                else if (line.find("\"startRotation\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3)
                    {
                        const float D2R = 3.14159265359f / 180.0f;
                        animData.startRot = Vec3(v[0] * D2R, v[1] * D2R, v[2] * D2R);
                    }
                }
                else if (line.find("\"endRotation\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3)
                    {
                        const float D2R = 3.14159265359f / 180.0f;
                        animData.endRot = Vec3(v[0] * D2R, v[1] * D2R, v[2] * D2R);
                    }
                }
                else if (line.find("\"startScale\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3) animData.startScale = Vec3(v[0], v[1], v[2]);
                }
                else if (line.find("\"endScale\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3) animData.endScale = Vec3(v[0], v[1], v[2]);
                }
                else if (line.find("\"center\"") != std::string::npos)
                {
                    auto v = parseArray(line);
                    if (v.size() >= 3) animData.center = Vec3(v[0], v[1], v[2]);
                }
                else if (line.find("\"radius\"") != std::string::npos)
                {
                    animData.radius = parseFloat(line);
                }
                else if (line.find("}") != std::string::npos)
                {
                    Animation *anim = new Animation();
                    anim->setLooping(animData.looping);

                    if (animData.type == "rotate")
                    {
                        anim->addKeyframe({0.0f, animData.startPos, animData.startRot, Vec3(1,1,1)});
                        anim->addKeyframe({animData.duration, animData.startPos, animData.endRot, Vec3(1,1,1)});
                    }
                    else if (animData.type == "scale")
                    {
                        anim->addKeyframe({0.0f, animData.startPos, Vec3(0,0,0), animData.startScale});
                        anim->addKeyframe({animData.duration / 2.0f, animData.startPos, Vec3(0,0,0), animData.endScale});
                        anim->addKeyframe({animData.duration, animData.startPos, Vec3(0,0,0), animData.startScale});
                    }
                    else if (animData.type == "move")
                    {
                        anim->addKeyframe({0.0f, animData.startPos, Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({animData.duration / 2.0f, animData.endPos, Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({animData.duration, animData.startPos, Vec3(0,0,0), Vec3(1,1,1)});
                    }
                    else if (animData.type == "moveAround")
                    {
                        float q = animData.duration / 4.0f;
                        Vec3 c = animData.center;
                        float r = animData.radius;
                        anim->addKeyframe({0.0f, c + Vec3(r,0,0), Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({q, c + Vec3(0,0,r), Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({2*q, c + Vec3(-r,0,0), Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({3*q, c + Vec3(0,0,-r), Vec3(0,0,0), Vec3(1,1,1)});
                        anim->addKeyframe({animData.duration, c + Vec3(r,0,0), Vec3(0,0,0), Vec3(1,1,1)});
                    }

                    animations[animData.name] = anim;

                    for (auto *obj : objects)
                    {
                        if (obj->getName() == animData.object)
                        {
                            obj->setAnimation(anim);
                            std::cerr << "Animation '" << animData.name << "' -> '" << animData.object << "'\n";
                            break;
                        }
                    }

                    buildingAnim = false;
                }
            }
        }
    }

    file.close();
    std::cerr << "Scene loaded from JSON\n";
    return true;
}

#endif
