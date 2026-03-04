#include <cstdint>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <map>
#include <random>
#include "helpers/math.hpp"
#include "helpers/primitives.hpp"
#include "helpers/animation.hpp"
#include "objects/cube.hpp"
#include "objects/sphere.hpp"
#include "objects/mesh.hpp"
#include "helpers/util.hpp"
#include "camera.hpp"
#include "helpers/json-loader.hpp"
#include "helpers/material-registry.hpp"
#include "helpers/bvh.hpp"
#include "helpers/texture.hpp"
#include "helpers/uv-mapping.hpp"
#include "lights.hpp"
#include <vector>
#include <cmath>

int g_shadowSamples = 32;
int g_maxBounces = 5;
bool g_forceFlatShading = false;
bool g_hardShadows = false;
bool g_disableFresnel = false;
bool g_disableToneMapping = false;

struct RenderSettings
{
    int width = 800;
    int height = 600;
    int samplesPerPixel = 1;
    bool useBVH = true;
    bool renderAnimation = false;
    int totalFrames = 125;
    int fps = 25;
    int shadowSamples = 32;
    int maxBounces = 5;
};

bool loadSettings(const char *filename, RenderSettings &settings)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Could not open settings file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.find("\"width\"") != std::string::npos)
        {
            settings.width = parseInt(line);
        }
        else if (line.find("\"height\"") != std::string::npos)
        {
            settings.height = parseInt(line);
        }
        else if (line.find("\"samplesPerPixel\"") != std::string::npos)
        {
            settings.samplesPerPixel = parseInt(line);
        }
        else if (line.find("\"useBVH\"") != std::string::npos)
        {
            settings.useBVH = (line.find("true") != std::string::npos);
        }
        else if (line.find("\"enabled\"") != std::string::npos)
        {
            settings.renderAnimation = (line.find("true") != std::string::npos);
        }
        else if (line.find("\"totalFrames\"") != std::string::npos)
        {
            settings.totalFrames = parseInt(line);
        }
        else if (line.find("\"fps\"") != std::string::npos)
        {
            settings.fps = parseInt(line);
        }
        else if (line.find("\"shadowSamples\"") != std::string::npos)
        {
            settings.shadowSamples = parseInt(line);
        }
        else if (line.find("\"maxBounces\"") != std::string::npos)
        {
            settings.maxBounces = parseInt(line);
        }
    }

    file.close();
    std::cout << "Settings loaded: " << settings.width << "x" << settings.height
              << ", " << settings.samplesPerPixel << " spp, BVH: " << (settings.useBVH ? "on" : "off")
              << ", Shadows: " << settings.shadowSamples << " samples, Bounces: " << settings.maxBounces
              << ", Animation: " << (settings.renderAnimation ? "on" : "off") << std::endl;
    return true;
}

inline float randomFloat()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(gen);
}

void renderAnimationSequence(int totalFrames, float frameDuration,
                             Camera *camera, std::vector<Object *> &objects,
                             const std::vector<PointLight> &pointLights,
                             const std::vector<DirectionalLight> &directionalLights,
                             const std::vector<SpotLight> &spotLights,
                             int width, int height, bool useBVH, int samplesPerPixel);

void renderSingleFrame(Camera *camera, std::vector<Object *> &objects,
                       const std::vector<PointLight> &pointLights,
                       const std::vector<DirectionalLight> &directionalLights,
                       const std::vector<SpotLight> &spotLights,
                       int width, int height, bool useBVH, int samplesPerPixel);

inline Vec3 acesToneMapping(const Vec3 &color)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    Vec3 result;
    result.x = (color.x * (a * color.x + b)) / (color.x * (c * color.x + d) + e);
    result.y = (color.y * (a * color.y + b)) / (color.y * (c * color.y + d) + e);
    result.z = (color.z * (a * color.z + b)) / (color.z * (c * color.z + d) + e);

    result.x = std::max(0.0f, std::min(1.0f, result.x));
    result.y = std::max(0.0f, std::min(1.0f, result.y));
    result.z = std::max(0.0f, std::min(1.0f, result.z));

    return result;
}

inline Vec3 reinhardToneMapping(const Vec3 &color)
{
    Vec3 result;
    result.x = color.x / (1.0f + color.x);
    result.y = color.y / (1.0f + color.y);
    result.z = color.z / (1.0f + color.z);
    return result;
}

inline Vec3 gammaCorrection(const Vec3 &color, float gamma = 2.2f)
{
    Vec3 result;
    result.x = std::pow(color.x, 1.0f / gamma);
    result.y = std::pow(color.y, 1.0f / gamma);
    result.z = std::pow(color.z, 1.0f / gamma);
    return result;
}

inline Vec3 reflect(const Vec3 &incident, const Vec3 &normal)
{
    return incident - normal * (2.0f * dot(incident, normal));
}

Vec3 traceReflection(Ray ray,
                     const BVH *bvh,
                     const std::vector<Triangle> &allTriangles,
                     const std::vector<Object *> &triangleOwners,
                     const std::vector<PointLight> &pointLights,
                     const std::vector<DirectionalLight> &directionalLights,
                     const std::vector<SpotLight> &spotLights,
                     int depth,
                     int maxDepth,
                     uint64_t &totalRaysShot,
                     uint64_t &totalIntersections,
                     bool useBVH,
                     int samplesPerPixel = 1);

Vec3 traceReflection(Ray ray,
                     const BVH *bvh,
                     const std::vector<Triangle> &allTriangles,
                     const std::vector<Object *> &triangleOwners,
                     const std::vector<PointLight> &pointLights,
                     const std::vector<DirectionalLight> &directionalLights,
                     const std::vector<SpotLight> &spotLights,
                     int depth,
                     int maxDepth,
                     uint64_t &totalRaysShot,
                     uint64_t &totalIntersections,
                     bool useBVH,
                     int samplesPerPixel)
{
    if (depth >= maxDepth)
    {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    bool anyHit = false;
    float closestT = 1e30f;
    Hit bestHit{};
    int bestTriIndex = -1;

    if (useBVH && bvh)
    {
        anyHit = bvh->traverse(ray, allTriangles, bestHit, bestTriIndex, closestT, totalIntersections);
    }
    else
    {
        for (size_t i = 0; i < allTriangles.size(); i++)
        {
            Hit hit = intersectTriangle(ray, allTriangles[i]);
            totalIntersections++;

            if (hit.hit && hit.t < closestT)
            {
                closestT = hit.t;
                bestHit = hit;
                bestTriIndex = i;
                anyHit = true;
            }
        }
    }

    if (!anyHit)
    {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    const Triangle &hitTri = allTriangles[bestTriIndex];
    Object *hitObject = triangleOwners[bestTriIndex];

    Vec3 hitPos = ray.origin + ray.dir * closestT;

    Vec3 normal = interpolateNormal(hitTri, bestHit.u, bestHit.v);

    if (dot(normal, ray.dir) > 0.0f)
        normal = normal * -1.0f;

    Vec3 viewDir = normalize(ray.origin - hitPos);
    const Material &mat = hitObject->getMaterial();

    float u = 0.0f, v = 0.0f;

    if (hitTri.hasUVs)
    {
        Vec2 interpUV = interpolateUV(hitTri, bestHit.u, bestHit.v);
        u = interpUV.u * mat.textureScale;
        v = interpUV.v * mat.textureScale;
    }
    else
    {
        std::string objName = hitObject->getName();

        if (objName.find("Sphere") != std::string::npos || objName.find("sphere") != std::string::npos)
        {
            Vec3 objPos = hitObject->getPosition();
            UVCoord uv = getSphereUV(hitPos, objPos, 1.0f);
            u = uv.u;
            v = uv.v;
        }
        else
        {
            // Use local-space coordinates for UV mapping (works regardless of rotation)
            Mat4 invMatrix = hitObject->getInverseMatrix();
            Vec3 localHitPos = invMatrix.transformPoint(hitPos);

            float tilesPerUnit = mat.textureScale * 0.1f;
            u = localHitPos.x * tilesPerUnit;
            v = localHitPos.z * tilesPerUnit;
        }
    }

    Vec3 specularOnly(0.0f, 0.0f, 0.0f);
    Vec3 directLight = calculatePhongWithTransparentShadows(hitPos, normal, viewDir, mat, u, v,
                                      pointLights, directionalLights, spotLights,
                                      bvh, allTriangles, triangleOwners, &specularOnly);

    if (mat.transparency > 0.0f)
    {
        bool entering = dot(ray.dir, normal) < 0.0f;
        Vec3 surfaceNormal = entering ? normal : normal * -1.0f;

        float n1 = entering ? 1.0f : mat.ior;
        float n2 = entering ? mat.ior : 1.0f;
        float eta = n1 / n2;

        float cosTheta = std::min(1.0f, -dot(ray.dir, surfaceNormal));

        float iorRatio = entering ? mat.ior : 1.0f / mat.ior;
        float r0 = (1.0f - iorRatio) / (1.0f + iorRatio);
        r0 = r0 * r0;
        float fresnelReflectance = g_disableFresnel ? r0 : fresnelSchlick(cosTheta, iorRatio);

        Vec3 refractDir = refract(ray.dir, surfaceNormal, eta);

        bool totalInternalReflection = (length(refractDir) < 0.001f);

        Vec3 reflectedColor(0.0f, 0.0f, 0.0f);
        Vec3 refractedColor(0.0f, 0.0f, 0.0f);

        Vec3 reflectDir = reflect(ray.dir, surfaceNormal);
        Ray reflectRay = {hitPos + surfaceNormal * RAY_ORIGIN_OFFSET, reflectDir};
        reflectedColor = traceReflection(reflectRay, bvh, allTriangles, triangleOwners,
                                         pointLights, directionalLights, spotLights,
                                         depth + 1, maxDepth, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);

        if (totalInternalReflection)
        {
            return reflectedColor;
        }
        else
        {
            // Offset in the opposite direction of normal to enter the material
            Vec3 refractOffset = surfaceNormal * -RAY_ORIGIN_OFFSET;
            Ray refractRay = {hitPos + refractOffset, normalize(refractDir)};

            refractedColor = traceReflection(refractRay, bvh, allTriangles, triangleOwners,
                                             pointLights, directionalLights, spotLights,
                                             depth + 1, maxDepth, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);

            // Apply Beer's law absorption when exiting the material
            if (!entering)
            {
                float distanceTraveled = closestT;
                Vec3 absorption = beerLambertAbsorption(mat.transmissionColor, distanceTraveled, mat.absorptionDistance);
                refractedColor.x *= absorption.x;
                refractedColor.y *= absorption.y;
                refractedColor.z *= absorption.z;
            }

            Vec3 transparentColor = reflectedColor * fresnelReflectance + refractedColor * (1.0f - fresnelReflectance);

            // Always add specular highlights on top of transparency blend
            return directLight * (1.0f - mat.transparency) + transparentColor * mat.transparency + specularOnly * mat.transparency;
        }
    }

    if (mat.reflectivity > 0.0f)
    {
        Vec3 reflectDir = reflect(ray.dir, normal);

        // Trace to find reflection distance (needed for distance-based glossiness)
        Ray testRay = {hitPos + normal * RAY_ORIGIN_OFFSET, reflectDir};
        float reflectionDist = 1e30f;

        if (useBVH && bvh)
        {
            Hit testHit{};
            int testIdx = -1;
            uint64_t testCount = 0;
            if (bvh->traverse(testRay, allTriangles, testHit, testIdx, reflectionDist, testCount))
            {
                reflectionDist = testHit.t;
            }
        }
        else
        {
            for (size_t i = 0; i < allTriangles.size(); i++)
            {
                Hit hit = intersectTriangle(testRay, allTriangles[i]);
                if (hit.hit && hit.t < reflectionDist)
                {
                    reflectionDist = hit.t;
                }
            }
        }

        // Perturb reflection direction for glossy surfaces (only with AA > 1)
        if (mat.glossiness < 1.0f && samplesPerPixel > 1)
        {
            Vec3 tangent, bitangent;
            buildOrthonormalBasis(reflectDir, tangent, bitangent);

            // Scale roughness by reflection distance so far reflections blur more
            float roughness = 1.0f - mat.glossiness;
            float distanceFactor = std::min(1.0f, reflectionDist / 20.0f);
            float effectiveRoughness = roughness * distanceFactor;

            float r1 = randomFloat() * 2.0f * 3.14159265f;
            float r2 = std::sqrt(randomFloat()) * effectiveRoughness * 0.3f;
            Vec3 offset = tangent * (std::cos(r1) * r2) + bitangent * (std::sin(r1) * r2);
            reflectDir = normalize(reflectDir + offset);
        }

        Ray reflectRay = {hitPos + normal * RAY_ORIGIN_OFFSET, reflectDir};

        Vec3 reflectedColor = traceReflection(reflectRay, bvh, allTriangles, triangleOwners,
                                              pointLights, directionalLights, spotLights,
                                              depth + 1, maxDepth, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);

        return directLight * (1.0f - mat.reflectivity) + reflectedColor * mat.reflectivity;
    }

    return directLight;
}

void loadDefaultScene(Camera *&camera,
                      std::vector<Object *> &objects,
                      std::vector<PointLight> &pointLights,
                      int width, int height)
{
    camera = new Camera(Vec3(3, 2, 3), Vec3(0, 0, 0), 60.0f, width, height);

    PointLight defaultLight;
    defaultLight.position = Vec3(2, 3, 1);
    defaultLight.color = Vec3(1, 1, 1);
    defaultLight.intensity = 5.0f;
    defaultLight.radius = 0.5f;
    pointLights.push_back(defaultLight);

    Cube *cube = new Cube();
    cube->setTransform(Vec3(0, 0, -2), Vec3(0, 0.785f, 0), Vec3(1, 1, 1));
    cube->setMaterial(MaterialRegistry::getMaterial("red"));
    objects.push_back(cube);
}

void renderFrame(const char *filename,
                 int width, int height,
                 Camera *camera,
                 const std::vector<Triangle> &allTriangles,
                 const std::vector<Object *> &triangleOwners,
                 const BVH *bvh,
                 const std::vector<PointLight> &pointLights,
                 const std::vector<DirectionalLight> &directionalLights,
                 const std::vector<SpotLight> &spotLights,
                 uint64_t &totalRaysShot,
                 uint64_t &totalIntersections,
                 bool useBVH,
                 int samplesPerPixel)
{
    std::ofstream out;
    if (!createPPM(filename, width, height, out))
    {
        std::cerr << "Failed to create frame file: " << filename << "\n";
        return;
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Vec3 color(0.0f, 0.0f, 0.0f);

            for (int s = 0; s < samplesPerPixel; s++)
            {
                float jitterX = randomFloat();
                float jitterY = randomFloat();

                Ray ray = camera->makeRay(x + jitterX, y + jitterY);
                totalRaysShot++;

                Vec3 sample = traceReflection(ray, bvh, allTriangles, triangleOwners,
                                              pointLights, directionalLights, spotLights,
                                              0, g_maxBounces, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);

                color = color + sample;
            }

            color = color * (1.0f / samplesPerPixel);

            Vec3 toneMapped = g_disableToneMapping ? color : acesToneMapping(color);

            toneMapped.x = std::max(0.0f, std::min(1.0f, toneMapped.x));
            toneMapped.y = std::max(0.0f, std::min(1.0f, toneMapped.y));
            toneMapped.z = std::max(0.0f, std::min(1.0f, toneMapped.z));

            uint8_t R = (uint8_t)(toneMapped.x * 255);
            uint8_t G = (uint8_t)(toneMapped.y * 255);
            uint8_t B = (uint8_t)(toneMapped.z * 255);

            out.put(R);
            out.put(G);
            out.put(B);
        }
    }

    out.close();
}

void renderAnimationSequence(int totalFrames, float frameDuration,
                             Camera *camera, std::vector<Object *> &objects,
                             const std::vector<PointLight> &pointLights,
                             const std::vector<DirectionalLight> &directionalLights,
                             const std::vector<SpotLight> &spotLights,
                             int width, int height, bool useBVH, int samplesPerPixel)
{
    std::cerr << "Starting animation render: " << totalFrames << " frames\n";

    uint64_t totalRaysShot = 0;
    uint64_t totalIntersections = 0;
    auto renderStart = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < totalFrames; frame++)
    {
        std::cerr << "Rendering frame " << frame + 1 << "/" << totalFrames << "\n";

        for (auto *obj : objects)
        {
            obj->updateAnimation(frameDuration);
        }

        std::vector<Triangle> allTriangles;
        std::vector<Object *> triangleOwners;

        for (auto *obj : objects)
        {
            auto tris = obj->getWorldTriangles();
            for (const auto &tri : tris)
            {
                allTriangles.push_back(tri);
                triangleOwners.push_back(obj);
            }
        }

        BVH bvh;
        if (useBVH)
        {
            bvh.build(allTriangles);
        }

        char frameName[256];
        snprintf(frameName, sizeof(frameName), "frames/out_%03d.ppm", frame);

        renderFrame(frameName, width, height, camera, allTriangles, triangleOwners,
                    &bvh, pointLights, directionalLights, spotLights, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);
    }

    auto renderEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEnd - renderStart);
    double durationSeconds = duration.count() / 1000.0;

    std::cerr << "\n=== Animation Render Complete ===\n";
    std::cerr << "Total frames: " << totalFrames << "\n";
    std::cerr << "Total rays shot: " << totalRaysShot << "\n";
    std::cerr << "Total intersections tested: " << totalIntersections << "\n";
    std::cerr << "Render time: " << durationSeconds << " s\n";
    std::cerr << "===================================\n";
}

void renderSingleFrame(Camera *camera, std::vector<Object *> &objects,
                       const std::vector<PointLight> &pointLights,
                       const std::vector<DirectionalLight> &directionalLights,
                       const std::vector<SpotLight> &spotLights,
                       int width, int height, bool useBVH, int samplesPerPixel)
{
    std::vector<Triangle> allTriangles;
    std::vector<Object *> triangleOwners;

    for (auto *obj : objects)
    {
        auto tris = obj->getWorldTriangles();
        for (const auto &tri : tris)
        {
            allTriangles.push_back(tri);
            triangleOwners.push_back(obj);
        }
    }

    uint64_t totalTriangles = allTriangles.size();
    uint64_t totalVertices = totalTriangles * 3;
    uint64_t totalObjects = objects.size();

    BVH bvh;
    auto bvhStart = std::chrono::high_resolution_clock::now();
    if (useBVH)
    {
        std::cerr << "Building BVH...\n";
        bvh.build(allTriangles);
    }
    auto bvhEnd = std::chrono::high_resolution_clock::now();
    auto bvhDuration = std::chrono::duration_cast<std::chrono::milliseconds>(bvhEnd - bvhStart);
    double bvhSeconds = bvhDuration.count() / 1000.0;

    uint64_t totalRaysShot = 0;
    uint64_t totalIntersections = 0;
    auto renderStart = std::chrono::high_resolution_clock::now();

    renderFrame("out.ppm", width, height, camera, allTriangles, triangleOwners,
                &bvh, pointLights, directionalLights, spotLights, totalRaysShot, totalIntersections, useBVH, samplesPerPixel);

    auto renderEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEnd - renderStart);
    double durationSeconds = duration.count() / 1000.0;

    std::cerr << "\n=== Scene Geometry ===\n";
    std::cerr << "Total objects: " << totalObjects << "\n";
    std::cerr << "Total triangles: " << totalTriangles << "\n";
    std::cerr << "Total vertices: " << totalVertices << "\n";

    if (useBVH)
    {
        std::cerr << "\n=== BVH Statistics ===\n";
        std::cerr << "BVH nodes: " << bvh.getNodeCount() << "\n";
        std::cerr << "BVH depth: " << bvh.getDepth() << "\n";
        std::cerr << "BVH build time: " << bvhSeconds << " s\n";
    }

    std::cerr << "\n=== Render Statistics ===\n";
    std::cerr << "Total rays shot: " << totalRaysShot << "\n";
    std::cerr << "Total intersections tested: " << totalIntersections << "\n";
    std::cerr << "Render time: " << durationSeconds << " s\n";
    std::cerr << "========================\n";
}

int main()
{
    RenderSettings settings;
    if (!loadSettings("settings.json", settings))
    {
        std::cerr << "Using default settings\n";
    }

    int width = settings.width;
    int height = settings.height;
    Camera *camera = nullptr;
    std::vector<Object *> objects;
    std::vector<PointLight> pointLights;
    std::vector<DirectionalLight> directionalLights;
    std::vector<SpotLight> spotLights;
    std::map<std::string, Animation *> animations;

    MaterialRegistry::initializeDefaults();

    TextureManager &tm = TextureManager::getInstance();

    Texture checker = Texture::createCheckerboard(256,
                                                  Vec3(0.9f, 0.9f, 0.9f), Vec3(0.1f, 0.1f, 0.1f));
    tm.addProceduralTexture("checkerboard", checker);

    if (!loadSceneJSON("scene.json", camera, objects, pointLights, directionalLights, spotLights, animations, width, height))
    {
        std::cerr << "Failed to load scene, using defaults\n";
        loadDefaultScene(camera, objects, pointLights, width, height);
    }

    bool renderAnimation = settings.renderAnimation;
    bool useBVH = settings.useBVH;
    int samplesPerPixel = settings.samplesPerPixel;
    int totalFrames = settings.totalFrames;
    float frameDuration = 1.0f / static_cast<float>(settings.fps);

    g_shadowSamples = settings.shadowSamples;
    g_maxBounces = settings.maxBounces;

    if (renderAnimation)
    {
        renderAnimationSequence(totalFrames, frameDuration, camera, objects,
                                pointLights, directionalLights, spotLights, width, height, useBVH, samplesPerPixel);
    }
    else
    {
        renderSingleFrame(camera, objects, pointLights, directionalLights, spotLights, width, height, useBVH, samplesPerPixel);
    }

    delete camera;
    for (auto *obj : objects)
        delete obj;
    return 0;
}
