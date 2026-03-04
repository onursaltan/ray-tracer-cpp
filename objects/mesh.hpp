#ifndef MESH_HPP
#define MESH_HPP

#include <string>
#include <vector>
#include "../helpers/math.hpp"
#include "../helpers/primitives.hpp"
#include "../helpers/obj-loader.hpp"
#include "object.hpp"

class Mesh : public Object
{
private:
    std::vector<Triangle> localTris;
    std::string filepath;
    std::string mtlFile;
    std::map<std::string, MTLMaterial> mtlMaterials;
    bool loaded;

public:
    Mesh() : Object(), loaded(false) {}

    Mesh(const std::string &objFilePath) : Object(), filepath(objFilePath), loaded(false)
    {
        loadFromFile(objFilePath);
    }

    bool loadFromFile(const std::string &objFilePath)
    {
        filepath = objFilePath;
        OBJMesh result = loadOBJ(objFilePath);

        if (!result.success)
        {
            std::cerr << "Failed to load mesh: " << result.error << "\n";
            loaded = false;
            return false;
        }

        localTris = result.triangles;
        mtlFile = result.mtlFile;
        mtlMaterials = result.materials;
        loaded = true;
        return true;
    }

    bool isLoaded() const { return loaded; }
    const std::string &getFilePath() const { return filepath; }
    const std::string &getMtlFile() const { return mtlFile; }
    const std::map<std::string, MTLMaterial> &getMtlMaterials() const { return mtlMaterials; }
    bool hasMtlMaterials() const { return !mtlMaterials.empty(); }
    size_t getTriangleCount() const { return localTris.size(); }

    std::vector<Triangle> getWorldTriangles() override
    {
        if (isDirty)
            updateModelMatrix();

        std::vector<Triangle> worldTris;
        worldTris.reserve(localTris.size());

        Mat4 normalMatrix = modelMatrix;

        bool uniformScale = (std::fabs(scale.x - scale.y) < 1e-6f &&
                             std::fabs(scale.y - scale.z) < 1e-6f);

        for (const auto &tri : localTris)
        {
            Triangle worldTri;

            worldTri.v0 = modelMatrix.transformPoint(tri.v0);
            worldTri.v1 = modelMatrix.transformPoint(tri.v1);
            worldTri.v2 = modelMatrix.transformPoint(tri.v2);

            if (tri.hasVertexNormals)
            {
                if (uniformScale)
                {
                    worldTri.n0 = normalize(modelMatrix.transformDir(tri.n0));
                    worldTri.n1 = normalize(modelMatrix.transformDir(tri.n1));
                    worldTri.n2 = normalize(modelMatrix.transformDir(tri.n2));
                }
                else
                {
                    Mat4 invMat = modelMatrix.inverse();
                    worldTri.n0 = normalize(Vec3(
                        invMat.m[0][0] * tri.n0.x + invMat.m[1][0] * tri.n0.y + invMat.m[2][0] * tri.n0.z,
                        invMat.m[0][1] * tri.n0.x + invMat.m[1][1] * tri.n0.y + invMat.m[2][1] * tri.n0.z,
                        invMat.m[0][2] * tri.n0.x + invMat.m[1][2] * tri.n0.y + invMat.m[2][2] * tri.n0.z));
                    worldTri.n1 = normalize(Vec3(
                        invMat.m[0][0] * tri.n1.x + invMat.m[1][0] * tri.n1.y + invMat.m[2][0] * tri.n1.z,
                        invMat.m[0][1] * tri.n1.x + invMat.m[1][1] * tri.n1.y + invMat.m[2][1] * tri.n1.z,
                        invMat.m[0][2] * tri.n1.x + invMat.m[1][2] * tri.n1.y + invMat.m[2][2] * tri.n1.z));
                    worldTri.n2 = normalize(Vec3(
                        invMat.m[0][0] * tri.n2.x + invMat.m[1][0] * tri.n2.y + invMat.m[2][0] * tri.n2.z,
                        invMat.m[0][1] * tri.n2.x + invMat.m[1][1] * tri.n2.y + invMat.m[2][1] * tri.n2.z,
                        invMat.m[0][2] * tri.n2.x + invMat.m[1][2] * tri.n2.y + invMat.m[2][2] * tri.n2.z));
                }
                worldTri.hasVertexNormals = true;
            }
            else
            {
                worldTri.hasVertexNormals = false;
            }

            worldTri.uv0 = tri.uv0;
            worldTri.uv1 = tri.uv1;
            worldTri.uv2 = tri.uv2;
            worldTri.hasUVs = tri.hasUVs;

            worldTris.push_back(worldTri);
        }

        return worldTris;
    }
};

#endif
