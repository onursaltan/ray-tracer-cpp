#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include <vector>
#include <random>
#include "helpers/math.hpp"
#include "helpers/primitives.hpp"
#include "helpers/bvh.hpp"
#include "helpers/texture.hpp"

extern int g_shadowSamples;
extern int g_maxBounces;
extern bool g_hardShadows;

struct PointLight
{
    Vec3 position;
    Vec3 color;
    float intensity;
    float radius;
};

struct DirectionalLight
{
    Vec3 direction;
    Vec3 color;
    float intensity;
};

struct SpotLight
{
    Vec3 position;
    Vec3 direction;
    Vec3 color;
    float intensity;
    float innerAngle;
    float outerAngle;
    float radius;
};

static thread_local std::mt19937 rng(std::random_device{}());
static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

Vec3 randomPointOnSphere(float radius)
{
    float rand1 = distribution(rng);
    float rand2 = distribution(rng);

    float theta = 2.0f * 3.14159265f * rand1;
    float phi = acos(2.0f * rand2 - 1.0f);

    float x = radius * sin(phi) * cos(theta);
    float y = radius * sin(phi) * sin(theta);
    float z = radius * cos(phi);

    return Vec3(x, y, z);
}

void buildOrthonormalBasis(const Vec3 &normal, Vec3 &tangent, Vec3 &bitangent)
{
    // Pick a vector not parallel to normal to avoid degenerate cross product
    Vec3 up = (std::abs(normal.y) < 0.999f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    tangent = normalize(cross(up, normal));
    bitangent = normalize(cross(normal, tangent));
}

class Object;

// Returns Vec3 transmission (colored shadow support for transparent surfaces)
Vec3 calculateShadowWithTransparency(Vec3 hitPos, Vec3 normal, Vec3 lightPos,
                                     const BVH *bvh,
                                     const std::vector<Triangle> &allTriangles,
                                     const std::vector<Object *> &triangleOwners,
                                     float lightRadius, int samples = -1)
{
    if (samples < 0)
        samples = g_shadowSamples;

    if (g_hardShadows)
        samples = 1;

    Vec3 totalTransmission(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < samples; i++)
    {
        Vec3 samplePos = g_hardShadows ? lightPos : lightPos + randomPointOnSphere(lightRadius);

        Vec3 toLight = samplePos - hitPos;
        float distToLight = length(toLight);
        Vec3 lightDir = normalize(toLight);

        Ray shadowRay{hitPos + normal * RAY_ORIGIN_OFFSET, lightDir};

        Vec3 transmission(1.0f, 1.0f, 1.0f);
        float currentT = 0.0f;
        float maxT = distToLight - 0.001f;

        // Trace through potentially multiple transparent surfaces
        int maxTransparentSurfaces = 8;
        for (int j = 0; j < maxTransparentSurfaces; j++)
        {
            Hit closestHit{};
            int closestIdx = -1;
            float closestT = maxT - currentT;

            if (bvh)
            {
                uint64_t dummyCount = 0;
                if (!bvh->traverse(shadowRay, allTriangles, closestHit, closestIdx, closestT, dummyCount))
                {
                    break;
                }
            }
            else
            {
                for (size_t k = 0; k < allTriangles.size(); k++)
                {
                    Hit hit = intersectTriangle(shadowRay, allTriangles[k]);
                    if (hit.hit && hit.t < closestT)
                    {
                        closestT = hit.t;
                        closestHit = hit;
                        closestIdx = (int)k;
                    }
                }
                if (closestIdx == -1)
                    break;
            }

            Object *hitObject = triangleOwners[closestIdx];
            const Material &mat = hitObject->getMaterial();

            if (mat.transparency > 0.0f)
            {
                float opacity = 1.0f - mat.transparency;

                // Account for Fresnel reflection reducing transmitted light
                Vec3 triNormal = normalize(cross(allTriangles[closestIdx].v1 - allTriangles[closestIdx].v0,
                                                    allTriangles[closestIdx].v2 - allTriangles[closestIdx].v0));
                float cosI = std::abs(dot(shadowRay.dir, triNormal));
                float fresnelRefl = fresnelSchlick(cosI, mat.ior);
                float fresnelTransmit = 1.0f - fresnelRefl;

                transmission.x *= fresnelTransmit * (mat.transparency * mat.transmissionColor.x + opacity * (1.0f - mat.diffuseColor.x * 0.5f));
                transmission.y *= fresnelTransmit * (mat.transparency * mat.transmissionColor.y + opacity * (1.0f - mat.diffuseColor.y * 0.5f));
                transmission.z *= fresnelTransmit * (mat.transparency * mat.transmissionColor.z + opacity * (1.0f - mat.diffuseColor.z * 0.5f));

                currentT += closestT + RAY_ORIGIN_OFFSET * 2.0f;
                shadowRay.origin = shadowRay.origin + shadowRay.dir * (closestT + RAY_ORIGIN_OFFSET * 2.0f);

                // Early out if transmission is negligible
                if (transmission.x < 0.01f && transmission.y < 0.01f && transmission.z < 0.01f)
                {
                    transmission = Vec3(0.0f, 0.0f, 0.0f);
                    break;
                }
            }
            else
            {
                transmission = Vec3(0.0f, 0.0f, 0.0f);
                break;
            }
        }

        totalTransmission = totalTransmission + transmission;
    }

    return totalTransmission * (1.0f / samples);
}

// Returns scalar shadow factor: 0 = fully shadowed, 1 = fully lit
float calculateShadow(Vec3 hitPos, Vec3 normal, Vec3 lightPos,
                      const BVH *bvh,
                      const std::vector<Triangle> &allTriangles,
                      float lightRadius, int samples = -1)
{
    if (samples < 0)
        samples = g_shadowSamples;

    if (g_hardShadows)
        samples = 1;

    float shadowFactor = 0.0f;

    for (int i = 0; i < samples; i++)
    {
        Vec3 samplePos = g_hardShadows ? lightPos : lightPos + randomPointOnSphere(lightRadius);

        Vec3 toLight = samplePos - hitPos;
        float distToLight = length(toLight);
        Vec3 lightDir = normalize(toLight);

        Ray shadowRay{hitPos + normal * RAY_ORIGIN_OFFSET, lightDir};

        bool inShadow = false;

        if (bvh)
        {
            Hit shadowHit{};
            int dummyIdx = -1;
            float maxDist = distToLight - 0.001f;
            uint64_t dummyCount = 0;

            inShadow = bvh->traverse(shadowRay, allTriangles, shadowHit, dummyIdx, maxDist, dummyCount);
        }
        else
        {
            for (const auto &tri : allTriangles)
            {
                Hit hit = intersectTriangle(shadowRay, tri);
                if (hit.hit && hit.t < distToLight - 0.001f)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        if (!inShadow)
            shadowFactor += 1.0f;
    }

    return shadowFactor / samples;
}

Vec3 calculatePhong(const Vec3 &hitPos,
                    const Vec3 &normal,
                    const Vec3 &viewDir,
                    const Material &material,
                    float u, float v,
                    const std::vector<PointLight> &pointLights,
                    const std::vector<DirectionalLight> &directionalLights,
                    const std::vector<SpotLight> &spotLights,
                    const BVH *bvh,
                    const std::vector<Triangle> &allTriangles)
{
    Vec3 diffuseColor = material.diffuseColor;

    if (material.hasTexture)
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *tex = tm.getTexture(material.diffuseTexture);
        if (tex)
        {
            diffuseColor = tex->sampleBilinear(u, v);
        }
    }

    // Normal mapping
    Vec3 perturbedNormal = normal;
    if (!material.normalTexture.empty())
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *normalTex = tm.getTexture(material.normalTexture);
        if (normalTex)
        {
            Vec3 texNormal = normalTex->sampleBilinear(u, v);

            // Convert from [0,1] to [-1,1] range (tangent-space normal map)
            texNormal = texNormal * 2.0f - Vec3(1, 1, 1);
            texNormal = normalize(texNormal);

            Vec3 tangent, bitangent;
            buildOrthonormalBasis(normal, tangent, bitangent);

            // Transform from tangent space to world space (Z maps to surface normal)
            perturbedNormal = normalize(
                tangent * texNormal.x +
                normal * texNormal.z +
                bitangent * texNormal.y);
        }
    }

    Vec3 finalNormal = perturbedNormal;

    // Roughness map modulates shininess: 0=smooth (high shininess), 1=rough (shininess->1)
    float shininess = material.shininess;
    if (!material.roughnessTexture.empty())
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *roughnessTex = tm.getTexture(material.roughnessTexture);
        if (roughnessTex)
        {
            Vec3 roughnessValue = roughnessTex->sample(u, v);
            float roughness = (roughnessValue.x + roughnessValue.y + roughnessValue.z) / 3.0f;
            shininess = material.shininess * (1.0f - roughness * 0.95f) + 1.0f;
        }
    }

    Vec3 ambient = Vec3(material.ambient, material.ambient, material.ambient);
    Vec3 color = Vec3(
        ambient.x * diffuseColor.x,
        ambient.y * diffuseColor.y,
        ambient.z * diffuseColor.z);

    // Point lights
    for (const auto &light : pointLights)
    {
        Vec3 lightDir = light.position - hitPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        float shadowFactor = calculateShadow(hitPos, finalNormal, light.position, bvh, allTriangles,
                                             light.radius, -1);

        float attenuation = light.intensity / (1.0f + distance * distance);

        float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
        Vec3 diffuseContrib = Vec3(
            diffuse * diffuseColor.x * light.color.x * attenuation * shadowFactor,
            diffuse * diffuseColor.y * light.color.y * attenuation * shadowFactor,
            diffuse * diffuseColor.z * light.color.z * attenuation * shadowFactor);

        Vec3 halfDir = normalize(lightDir + viewDir);
        float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
        Vec3 specularContrib = Vec3(
            spec * material.specularColor.x * light.color.x * attenuation * shadowFactor,
            spec * material.specularColor.y * light.color.y * attenuation * shadowFactor,
            spec * material.specularColor.z * light.color.z * attenuation * shadowFactor);

        color = color + diffuseContrib + specularContrib;
    }

    // Spot lights
    for (const auto &light : spotLights)
    {
        Vec3 lightDir = light.position - hitPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        Vec3 spotDir = normalize(light.direction);
        float theta = dot(lightDir * -1.0f, spotDir);

        // Smooth falloff between inner and outer cone
        float epsilon = cos(light.innerAngle) - cos(light.outerAngle);
        float spotIntensity = std::max(0.0f, (float)((theta - cos(light.outerAngle)) / epsilon));
        spotIntensity = std::min(1.0f, spotIntensity);

        if (spotIntensity > 0.0f)
        {
            float shadowFactor = calculateShadow(hitPos, finalNormal, light.position, bvh, allTriangles,
                                                 light.radius, -1);

            float attenuation = light.intensity / (1.0f + distance * distance);
            attenuation *= spotIntensity;

            float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
            Vec3 diffuseContrib = Vec3(
                diffuse * diffuseColor.x * light.color.x * attenuation * shadowFactor,
                diffuse * diffuseColor.y * light.color.y * attenuation * shadowFactor,
                diffuse * diffuseColor.z * light.color.z * attenuation * shadowFactor);

            Vec3 halfDir = normalize(lightDir + viewDir);
            float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
            Vec3 specularContrib = Vec3(
                spec * material.specularColor.x * light.color.x * attenuation * shadowFactor,
                spec * material.specularColor.y * light.color.y * attenuation * shadowFactor,
                spec * material.specularColor.z * light.color.z * attenuation * shadowFactor);

            color = color + diffuseContrib + specularContrib;
        }
    }

    // Directional lights
    for (const auto &light : directionalLights)
    {
        Vec3 lightDir = normalize(light.direction * -1.0f);

        Ray shadowRay{hitPos + normal * RAY_ORIGIN_OFFSET, lightDir};
        bool inShadow = false;
        if (bvh)
        {
            Hit shadowHit{};
            int shadowIdx = -1;
            float shadowT = 1e30f;
            uint64_t dummyCount = 0;
            if (bvh->traverse(shadowRay, allTriangles, shadowHit, shadowIdx, shadowT, dummyCount))
            {
                inShadow = true;
            }
        }
        else
        {
            for (size_t k = 0; k < allTriangles.size(); k++)
            {
                Hit hit = intersectTriangle(shadowRay, allTriangles[k]);
                if (hit.hit && hit.t > RAY_EPSILON)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        if (inShadow)
            continue;

        float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
        Vec3 diffuseContrib = Vec3(
            diffuse * diffuseColor.x * light.color.x * light.intensity,
            diffuse * diffuseColor.y * light.color.y * light.intensity,
            diffuse * diffuseColor.z * light.color.z * light.intensity);

        Vec3 halfDir = normalize(lightDir + viewDir);
        float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
        Vec3 specularContrib = Vec3(
            spec * material.specularColor.x * light.color.x * light.intensity,
            spec * material.specularColor.y * light.color.y * light.intensity,
            spec * material.specularColor.z * light.color.z * light.intensity);

        color = color + diffuseContrib + specularContrib;
    }

    color.x = std::min(1.0f, color.x);
    color.y = std::min(1.0f, color.y);
    color.z = std::min(1.0f, color.z);

    return color;
}

Vec3 calculatePhongWithTransparentShadows(const Vec3 &hitPos,
                                          const Vec3 &normal,
                                          const Vec3 &viewDir,
                                          const Material &material,
                                          float u, float v,
                                          const std::vector<PointLight> &pointLights,
                                          const std::vector<DirectionalLight> &directionalLights,
                                          const std::vector<SpotLight> &spotLights,
                                          const BVH *bvh,
                                          const std::vector<Triangle> &allTriangles,
                                          const std::vector<Object *> &triangleOwners,
                                          Vec3 *outSpecular = nullptr)
{
    Vec3 diffuseColor = material.diffuseColor;

    if (material.hasTexture)
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *tex = tm.getTexture(material.diffuseTexture);
        if (tex)
        {
            diffuseColor = tex->sampleBilinear(u, v);
        }
    }

    // Normal mapping
    Vec3 perturbedNormal = normal;
    if (!material.normalTexture.empty())
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *normalTex = tm.getTexture(material.normalTexture);
        if (normalTex)
        {
            Vec3 texNormal = normalTex->sampleBilinear(u, v);
            texNormal = texNormal * 2.0f - Vec3(1, 1, 1);
            texNormal = normalize(texNormal);
            Vec3 tangent, bitangent;
            buildOrthonormalBasis(normal, tangent, bitangent);
            perturbedNormal = normalize(
                tangent * texNormal.x +
                normal * texNormal.z +
                bitangent * texNormal.y);
        }
    }

    Vec3 finalNormal = perturbedNormal;

    // Roughness map modulates shininess: 0=smooth (high shininess), 1=rough (shininess->1)
    float shininess = material.shininess;
    if (!material.roughnessTexture.empty())
    {
        TextureManager &tm = TextureManager::getInstance();
        const Texture *roughnessTex = tm.getTexture(material.roughnessTexture);
        if (roughnessTex)
        {
            Vec3 roughnessValue = roughnessTex->sample(u, v);
            float roughness = (roughnessValue.x + roughnessValue.y + roughnessValue.z) / 3.0f;
            shininess = material.shininess * (1.0f - roughness * 0.95f) + 1.0f;
        }
    }

    Vec3 ambient = Vec3(material.ambient, material.ambient, material.ambient);
    Vec3 color = Vec3(
        ambient.x * diffuseColor.x,
        ambient.y * diffuseColor.y,
        ambient.z * diffuseColor.z);

    Vec3 totalSpecular(0.0f, 0.0f, 0.0f);

    // Point lights
    for (const auto &light : pointLights)
    {
        Vec3 lightDir = light.position - hitPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        Vec3 shadowTransmission = calculateShadowWithTransparency(hitPos, finalNormal, light.position,
                                                                   bvh, allTriangles, triangleOwners,
                                                                   light.radius, -1);

        float attenuation = light.intensity / (1.0f + distance * distance);

        float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
        Vec3 diffuseContrib = Vec3(
            diffuse * diffuseColor.x * light.color.x * attenuation * shadowTransmission.x,
            diffuse * diffuseColor.y * light.color.y * attenuation * shadowTransmission.y,
            diffuse * diffuseColor.z * light.color.z * attenuation * shadowTransmission.z);

        Vec3 halfDir = normalize(lightDir + viewDir);
        float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
        Vec3 specularContrib = Vec3(
            spec * material.specularColor.x * light.color.x * attenuation * shadowTransmission.x,
            spec * material.specularColor.y * light.color.y * attenuation * shadowTransmission.y,
            spec * material.specularColor.z * light.color.z * attenuation * shadowTransmission.z);

        color = color + diffuseContrib + specularContrib;
        totalSpecular = totalSpecular + specularContrib;
    }

    // Spot lights
    for (const auto &light : spotLights)
    {
        Vec3 lightDir = light.position - hitPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        Vec3 spotDir = normalize(light.direction);
        float theta = dot(lightDir * -1.0f, spotDir);
        float epsilon = cos(light.innerAngle) - cos(light.outerAngle);
        float spotIntensity = std::max(0.0f, (float)((theta - cos(light.outerAngle)) / epsilon));
        spotIntensity = std::min(1.0f, spotIntensity);

        if (spotIntensity > 0.0f)
        {
            Vec3 shadowTransmission = calculateShadowWithTransparency(hitPos, finalNormal, light.position,
                                                                       bvh, allTriangles, triangleOwners,
                                                                       light.radius, -1);

            float attenuation = light.intensity / (1.0f + distance * distance);
            attenuation *= spotIntensity;

            float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
            Vec3 diffuseContrib = Vec3(
                diffuse * diffuseColor.x * light.color.x * attenuation * shadowTransmission.x,
                diffuse * diffuseColor.y * light.color.y * attenuation * shadowTransmission.y,
                diffuse * diffuseColor.z * light.color.z * attenuation * shadowTransmission.z);

            Vec3 halfDir = normalize(lightDir + viewDir);
            float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
            Vec3 specularContrib = Vec3(
                spec * material.specularColor.x * light.color.x * attenuation * shadowTransmission.x,
                spec * material.specularColor.y * light.color.y * attenuation * shadowTransmission.y,
                spec * material.specularColor.z * light.color.z * attenuation * shadowTransmission.z);

            color = color + diffuseContrib + specularContrib;
            totalSpecular = totalSpecular + specularContrib;
        }
    }

    // Directional lights
    for (const auto &light : directionalLights)
    {
        Vec3 lightDir = normalize(light.direction * -1.0f);

        Ray shadowRay{hitPos + normal * RAY_ORIGIN_OFFSET, lightDir};
        bool inShadow = false;
        if (bvh)
        {
            Hit shadowHit{};
            int shadowIdx = -1;
            float shadowT = 1e30f;
            uint64_t dummyCount = 0;
            if (bvh->traverse(shadowRay, allTriangles, shadowHit, shadowIdx, shadowT, dummyCount))
            {
                inShadow = true;
            }
        }
        else
        {
            for (size_t k = 0; k < allTriangles.size(); k++)
            {
                Hit hit = intersectTriangle(shadowRay, allTriangles[k]);
                if (hit.hit && hit.t > RAY_EPSILON)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        if (inShadow)
            continue;

        float diffuse = std::max(0.0f, dot(finalNormal, lightDir));
        Vec3 diffuseContrib = Vec3(
            diffuse * diffuseColor.x * light.color.x * light.intensity,
            diffuse * diffuseColor.y * light.color.y * light.intensity,
            diffuse * diffuseColor.z * light.color.z * light.intensity);

        Vec3 halfDir = normalize(lightDir + viewDir);
        float spec = std::pow(std::max(0.0f, dot(finalNormal, halfDir)), shininess);
        Vec3 specularContrib = Vec3(
            spec * material.specularColor.x * light.color.x * light.intensity,
            spec * material.specularColor.y * light.color.y * light.intensity,
            spec * material.specularColor.z * light.color.z * light.intensity);

        color = color + diffuseContrib + specularContrib;
        totalSpecular = totalSpecular + specularContrib;
    }

    if (outSpecular)
        *outSpecular = totalSpecular;

    color.x = std::min(1.0f, color.x);
    color.y = std::min(1.0f, color.y);
    color.z = std::min(1.0f, color.z);

    return color;
}

#endif
