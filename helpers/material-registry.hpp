#ifndef MATERIAL_REGISTRY_HPP
#define MATERIAL_REGISTRY_HPP

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include "primitives.hpp"
#include "texture.hpp"

struct MTLMaterial;

static std::string parseStringValue(const std::string &str)
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

class MaterialRegistry
{
private:
    static std::unordered_map<std::string, Material> materials;

public:
    static void registerMaterial(const std::string &name, const Material &mat)
    {
        materials[name] = mat;
    }

    static Material getMaterial(const std::string &name)
    {
        if (materials.find(name) != materials.end())
        {
            return materials[name];
        }
        Material defaultMat;
        defaultMat.diffuseColor = Vec3(1.0f, 1.0f, 1.0f);
        defaultMat.specularColor = Vec3(0.5f, 0.5f, 0.5f);
        defaultMat.shininess = 16.0f;
        defaultMat.ambient = 0.1f;
        defaultMat.reflectivity = 0.0f;
        return defaultMat;
    }

    static void initializeDefaults()
    {
        loadFromJSON("materials.json");
        loadFromMTL("materials.mtl");
    }

    static void loadFromMTL(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file)
        {
            return;
        }
        file.close();

        std::ifstream mtlFile(filename);
        std::string currentName;
        Material mat;
        bool hasMaterial = false;

        std::string line;
        while (std::getline(mtlFile, line))
        {
            if (line.empty())
                continue;

            // Custom extensions stored in MTL comments (e.g. '# reflectivity 0.5')
            if (line[0] == '#')
            {
                if (line.size() > 2)
                {
                    std::istringstream iss(line.substr(2));
                    std::string key;
                    iss >> key;
                    if (key == "reflectivity")
                    {
                        iss >> mat.reflectivity;
                    }
                    else if (key == "glossiness")
                    {
                        iss >> mat.glossiness;
                    }
                }
                continue;
            }

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "newmtl")
            {
                if (hasMaterial && !currentName.empty())
                {
                    registerMaterial(currentName, mat);
                }
                iss >> currentName;
                mat = Material();
                hasMaterial = true;
            }
            else if (prefix == "Ka")
            {
                float r, g, b;
                iss >> r >> g >> b;
                // Average Ka for ambient scalar
                mat.ambient = (r + g + b) / 3.0f;
            }
            else if (prefix == "Kd")
            {
                float r, g, b;
                iss >> r >> g >> b;
                mat.diffuseColor = Vec3(r, g, b);
            }
            else if (prefix == "Ks")
            {
                float r, g, b;
                iss >> r >> g >> b;
                mat.specularColor = Vec3(r, g, b);
            }
            else if (prefix == "Ns")
            {
                iss >> mat.shininess;
            }
            else if (prefix == "map_Kd")
            {
                iss >> mat.diffuseTexture;
                mat.hasTexture = !mat.diffuseTexture.empty();
            }
            else if (prefix == "map_Bump" || prefix == "bump")
            {
                iss >> mat.normalTexture;
            }
        }

        if (hasMaterial && !currentName.empty())
        {
            registerMaterial(currentName, mat);
        }

        mtlFile.close();

        TextureManager &tm = TextureManager::getInstance();
        for (const auto &entry : materials)
        {
            const Material &m = entry.second;
            if (m.hasTexture && !m.diffuseTexture.empty() && !tm.hasTexture(m.diffuseTexture))
            {
                tm.loadTexture(m.diffuseTexture, m.diffuseTexture);
            }
            if (!m.normalTexture.empty() && !tm.hasTexture(m.normalTexture))
            {
                tm.loadTexture(m.normalTexture, m.normalTexture);
            }
        }

        std::cerr << "Loaded materials from MTL: " << filename << "\n";
    }

    static void registerMaterialsFromMTL(const std::map<std::string, MTLMaterial> &mtlMaterials)
    {
        for (const auto &entry : mtlMaterials)
        {
            const MTLMaterial &mtl = entry.second;
            Material mat;
            mat.diffuseColor = mtl.diffuse;
            mat.specularColor = mtl.specular;
            mat.shininess = mtl.shininess;
            mat.ambient = (mtl.ambient.x + mtl.ambient.y + mtl.ambient.z) / 3.0f;
            mat.reflectivity = mtl.reflectivity;
            mat.glossiness = mtl.glossiness;
            mat.diffuseTexture = mtl.diffuseTexture;
            mat.normalTexture = mtl.normalTexture;
            mat.hasTexture = !mtl.diffuseTexture.empty();

            registerMaterial(entry.first, mat);
        }
    }

    static void loadFromJSON(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << "Warning: Could not open " << filename << ", using hardcoded defaults\n";
            loadHardcodedDefaults();
            return;
        }

        std::string line;
        std::string currentMaterial;
        Material mat;
        int valueIndex = 0;

        while (std::getline(file, line))
        {
            line.erase(0, line.find_first_not_of(" \t"));

            if (line.find("\"") != std::string::npos && line.find(":") != std::string::npos)
            {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start);
                if (end != std::string::npos)
                {
                    std::string key = line.substr(start, end - start);

                    // Identify material names by excluding known property keys
                    if (key != "diffuse" && key != "specular" && key != "shininess" &&
                        key != "ambient" && key != "reflectivity" && key != "glossiness" &&
                        key != "materials" && key != "diffuseTexture" &&
                        key != "normalTexture" && key != "roughnessTexture" &&
                        key != "transparency" && key != "ior" &&
                        key != "transmissionColor" && key != "absorptionDistance" &&
                        key != "textureScale")
                    {
                        if (!currentMaterial.empty())
                        {
                            registerMaterial(currentMaterial, mat);
                        }
                        currentMaterial = key;
                        mat = Material();
                        valueIndex = 0;
                    }
                    else if (key == "diffuse" && line.find("[") != std::string::npos)
                    {
                        size_t arrStart = line.find("[") + 1;
                        size_t arrEnd = line.find("]");
                        std::string values = line.substr(arrStart, arrEnd - arrStart);
                        std::istringstream iss(values);
                        float r, g, b;
                        char comma;
                        iss >> r >> comma >> g >> comma >> b;
                        mat.diffuseColor = Vec3(r, g, b);
                    }
                    else if (key == "specular" && line.find("[") != std::string::npos)
                    {
                        size_t arrStart = line.find("[") + 1;
                        size_t arrEnd = line.find("]");
                        std::string values = line.substr(arrStart, arrEnd - arrStart);
                        std::istringstream iss(values);
                        float r, g, b;
                        char comma;
                        iss >> r >> comma >> g >> comma >> b;
                        mat.specularColor = Vec3(r, g, b);
                    }
                    else if (key == "shininess")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.shininess = std::stof(value);
                    }
                    else if (key == "ambient")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.ambient = std::stof(value);
                    }
                    else if (key == "reflectivity")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.reflectivity = std::stof(value);
                    }
                    else if (key == "glossiness")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.glossiness = std::stof(value);
                    }
                    else if (key == "transparency")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.transparency = std::stof(value);
                    }
                    else if (key == "ior")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.ior = std::stof(value);
                    }
                    else if (key == "transmissionColor" && line.find("[") != std::string::npos)
                    {
                        size_t arrStart = line.find("[") + 1;
                        size_t arrEnd = line.find("]");
                        std::string values = line.substr(arrStart, arrEnd - arrStart);
                        std::istringstream iss(values);
                        float r, g, b;
                        char comma;
                        iss >> r >> comma >> g >> comma >> b;
                        mat.transmissionColor = Vec3(r, g, b);
                    }
                    else if (key == "absorptionDistance")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.absorptionDistance = std::stof(value);
                    }
                    else if (key == "diffuseTexture")
                    {
                        mat.diffuseTexture = parseStringValue(line);
                        mat.hasTexture = !mat.diffuseTexture.empty();
                    }
                    else if (key == "normalTexture")
                    {
                        mat.normalTexture = parseStringValue(line);
                    }
                    else if (key == "roughnessTexture")
                    {
                        mat.roughnessTexture = parseStringValue(line);
                    }
                    else if (key == "textureScale")
                    {
                        size_t colonPos = line.find(":");
                        std::string value = line.substr(colonPos + 1);
                        mat.textureScale = std::stof(value);
                    }
                }
            }
        }

        if (!currentMaterial.empty())
        {
            registerMaterial(currentMaterial, mat);
        }

        file.close();

        TextureManager &tm = TextureManager::getInstance();
        for (const auto &entry : materials)
        {
            const Material &mat = entry.second;

            if (mat.hasTexture && !mat.diffuseTexture.empty())
            {
                if (!tm.hasTexture(mat.diffuseTexture))
                {
                    tm.loadTexture(mat.diffuseTexture, mat.diffuseTexture);
                }
            }

            if (!mat.normalTexture.empty())
            {
                if (!tm.hasTexture(mat.normalTexture))
                {
                    tm.loadTexture(mat.normalTexture, mat.normalTexture);
                }
            }

            if (!mat.roughnessTexture.empty())
            {
                if (!tm.hasTexture(mat.roughnessTexture))
                {
                    tm.loadTexture(mat.roughnessTexture, mat.roughnessTexture);
                }
            }
        }

        std::cerr << "Loaded " << materials.size() << " materials from " << filename << "\n";
    }

private:
    static void loadHardcodedDefaults()
    {
        Material red;
        red.diffuseColor = Vec3(0.75f, 0.1f, 0.1f);
        red.specularColor = Vec3(0.05f, 0.05f, 0.05f);
        red.shininess = 5.0f;
        red.ambient = 0.1f;
        registerMaterial("red", red);

        Material blue;
        blue.diffuseColor = Vec3(0.1f, 0.1f, 0.75f);
        blue.specularColor = Vec3(0.05f, 0.05f, 0.05f);
        blue.shininess = 5.0f;
        blue.ambient = 0.1f;
        registerMaterial("blue", blue);

        Material green;
        green.diffuseColor = Vec3(0.1f, 0.75f, 0.1f);
        green.specularColor = Vec3(0.05f, 0.05f, 0.05f);
        green.shininess = 5.0f;
        green.ambient = 0.1f;
        registerMaterial("green", green);

        Material white;
        white.diffuseColor = Vec3(0.9f, 0.9f, 0.9f);
        white.specularColor = Vec3(0.1f, 0.1f, 0.1f);
        white.shininess = 10.0f;
        white.ambient = 0.1f;
        registerMaterial("white", white);

        Material metal;
        metal.diffuseColor = Vec3(0.5f, 0.5f, 0.5f);
        metal.specularColor = Vec3(1.0f, 1.0f, 1.0f);
        metal.shininess = 128.0f;
        metal.ambient = 0.2f;
        registerMaterial("metal", metal);

        Material gold;
        gold.diffuseColor = Vec3(0.8f, 0.7f, 0.2f);
        gold.specularColor = Vec3(1.0f, 1.0f, 0.8f);
        gold.shininess = 96.0f;
        gold.ambient = 0.15f;
        registerMaterial("gold", gold);
    }

public:
    static bool materialExists(const std::string &name)
    {
        return materials.find(name) != materials.end();
    }
};

std::unordered_map<std::string, Material> MaterialRegistry::materials;

#endif
