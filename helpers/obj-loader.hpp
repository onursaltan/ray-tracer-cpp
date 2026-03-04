#ifndef OBJ_LOADER_HPP
#define OBJ_LOADER_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "math.hpp"
#include "primitives.hpp"

struct MTLMaterial
{
    std::string name;
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    float shininess;
    float dissolve;
    float reflectivity;
    float glossiness;
    std::string diffuseTexture;
    std::string normalTexture;
    int illum;

    MTLMaterial() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.8f, 0.8f, 0.8f),
                    specular(1.0f, 1.0f, 1.0f), shininess(32.0f),
                    dissolve(1.0f), reflectivity(0.0f), glossiness(1.0f), illum(2) {}
};

struct MTLFile
{
    std::map<std::string, MTLMaterial> materials;
    bool success;
    std::string error;
};

inline MTLFile loadMTL(const std::string &filename)
{
    MTLFile result;
    result.success = false;

    std::ifstream file(filename);
    if (!file.is_open())
    {
        result.error = "Could not open MTL file: " + filename;
        return result;
    }

    MTLMaterial currentMat;
    std::string currentName;
    bool hasMaterial = false;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            if (line.size() > 2 && line[0] == '#' && line[1] == ' ')
            {
                std::istringstream iss(line.substr(2));
                std::string key;
                iss >> key;
                if (key == "reflectivity")
                {
                    iss >> currentMat.reflectivity;
                }
                else if (key == "glossiness")
                {
                    iss >> currentMat.glossiness;
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
                result.materials[currentName] = currentMat;
            }
            iss >> currentName;
            currentMat = MTLMaterial();
            currentMat.name = currentName;
            hasMaterial = true;
        }
        else if (prefix == "Ka")
        {
            float r, g, b;
            iss >> r >> g >> b;
            currentMat.ambient = Vec3(r, g, b);
        }
        else if (prefix == "Kd")
        {
            float r, g, b;
            iss >> r >> g >> b;
            currentMat.diffuse = Vec3(r, g, b);
        }
        else if (prefix == "Ks")
        {
            float r, g, b;
            iss >> r >> g >> b;
            currentMat.specular = Vec3(r, g, b);
        }
        else if (prefix == "Ns")
        {
            iss >> currentMat.shininess;
        }
        else if (prefix == "d")
        {
            iss >> currentMat.dissolve;
        }
        else if (prefix == "Tr")
        {
            float tr;
            iss >> tr;
            currentMat.dissolve = 1.0f - tr;
        }
        else if (prefix == "illum")
        {
            iss >> currentMat.illum;
        }
        else if (prefix == "map_Kd")
        {
            iss >> currentMat.diffuseTexture;
        }
        else if (prefix == "map_Bump" || prefix == "bump")
        {
            iss >> currentMat.normalTexture;
        }
    }

    if (hasMaterial && !currentName.empty())
    {
        result.materials[currentName] = currentMat;
    }

    file.close();
    result.success = true;

    std::cerr << "Loaded MTL: " << filename << " (" << result.materials.size() << " materials)\n";
    return result;
}

inline std::string getDirectory(const std::string &filepath)
{
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        return filepath.substr(0, lastSlash + 1);
    }
    return "";
}

struct OBJMesh
{
    std::vector<Triangle> triangles;
    std::string mtlFile;
    std::map<std::string, MTLMaterial> materials;
    bool success;
    std::string error;
};

struct FaceVertex
{
    int v;
    int vt;
    int vn;
};

inline FaceVertex parseFaceVertex(const std::string &token, int vertexCount, int texCoordCount, int normalCount)
{
    FaceVertex fv = {0, 0, 0};

    std::istringstream iss(token);
    std::string part;
    int partIndex = 0;

    while (std::getline(iss, part, '/'))
    {
        if (!part.empty())
        {
            int idx = std::stoi(part);

            switch (partIndex)
            {
            case 0:
                fv.v = (idx < 0) ? vertexCount + idx + 1 : idx;
                break;
            case 1:
                fv.vt = (idx < 0) ? texCoordCount + idx + 1 : idx;
                break;
            case 2:
                fv.vn = (idx < 0) ? normalCount + idx + 1 : idx;
                break;
            }
        }
        partIndex++;
    }

    return fv;
}

inline void computeSmoothNormals(std::vector<Vec3> &vertices,
                                 std::vector<std::vector<int>> &vertexFaces,
                                 const std::vector<Triangle> &triangles,
                                 std::vector<Vec3> &outNormals)
{
    outNormals.resize(vertices.size(), Vec3(0, 0, 0));

    for (size_t i = 0; i < triangles.size(); i++)
    {
        Vec3 faceN = faceNormal(triangles[i]);

        for (int vi : vertexFaces[i])
        {
            if (vi >= 0 && vi < (int)outNormals.size())
            {
                outNormals[vi] = outNormals[vi] + faceN;
            }
        }
    }

    for (auto &n : outNormals)
    {
        n = normalize(n);
    }
}

inline OBJMesh loadOBJ(const std::string &filename)
{
    OBJMesh result;
    result.success = false;

    std::ifstream file(filename);
    if (!file.is_open())
    {
        result.error = "Could not open file: " + filename;
        return result;
    }

    std::string directory = getDirectory(filename);

    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;

    struct RawFace
    {
        std::vector<FaceVertex> verts;
    };
    std::vector<RawFace> rawFaces;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "mtllib")
        {
            std::string mtlFilename;
            iss >> mtlFilename;
            result.mtlFile = directory + mtlFilename;

            MTLFile mtlResult = loadMTL(result.mtlFile);
            if (mtlResult.success)
            {
                result.materials = mtlResult.materials;
            }
        }
        else if (prefix == "v")
        {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vec3(x, y, z));
        }
        else if (prefix == "vn")
        {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(normalize(Vec3(x, y, z)));
        }
        else if (prefix == "vt")
        {
            float u, v;
            iss >> u >> v;
            texCoords.push_back(Vec2(u, v));
        }
        else if (prefix == "f")
        {
            std::vector<FaceVertex> faceVerts;
            std::string token;

            while (iss >> token)
            {
                FaceVertex fv = parseFaceVertex(token,
                                                (int)vertices.size(),
                                                (int)texCoords.size(),
                                                (int)normals.size());
                faceVerts.push_back(fv);
            }

            if (faceVerts.size() >= 3)
            {
                RawFace rf;
                rf.verts = faceVerts;
                rawFaces.push_back(rf);
            }
        }
    }

    file.close();

    if (vertices.empty())
    {
        result.error = "No vertices found in OBJ file";
        return result;
    }

    bool hasFileNormals = !normals.empty();
    bool hasFileTexCoords = !texCoords.empty();

    std::vector<Vec3> smoothNormals;
    std::map<int, Vec3> vertexNormalAccum;

    if (!hasFileNormals)
    {
        for (size_t i = 0; i < vertices.size(); i++)
        {
            vertexNormalAccum[(int)i] = Vec3(0, 0, 0);
        }

        for (const auto &rf : rawFaces)
        {
            if (rf.verts.size() < 3)
                continue;

            int i0 = rf.verts[0].v - 1;
            int i1 = rf.verts[1].v - 1;
            int i2 = rf.verts[2].v - 1;

            if (i0 < 0 || i0 >= (int)vertices.size() ||
                i1 < 0 || i1 >= (int)vertices.size() ||
                i2 < 0 || i2 >= (int)vertices.size())
                continue;

            Vec3 e1 = vertices[i1] - vertices[i0];
            Vec3 e2 = vertices[i2] - vertices[i0];
            Vec3 fn = normalize(cross(e1, e2));

            for (const auto &fv : rf.verts)
            {
                int vi = fv.v - 1;
                if (vi >= 0 && vi < (int)vertices.size())
                {
                    vertexNormalAccum[vi] = vertexNormalAccum[vi] + fn;
                }
            }
        }

        smoothNormals.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++)
        {
            smoothNormals[i] = normalize(vertexNormalAccum[(int)i]);
        }
    }

    for (const auto &rf : rawFaces)
    {
        if (rf.verts.size() < 3)
            continue;

        for (size_t i = 1; i + 1 < rf.verts.size(); i++)
        {
            const FaceVertex &fv0 = rf.verts[0];
            const FaceVertex &fv1 = rf.verts[i];
            const FaceVertex &fv2 = rf.verts[i + 1];

            int vi0 = fv0.v - 1;
            int vi1 = fv1.v - 1;
            int vi2 = fv2.v - 1;

            if (vi0 < 0 || vi0 >= (int)vertices.size() ||
                vi1 < 0 || vi1 >= (int)vertices.size() ||
                vi2 < 0 || vi2 >= (int)vertices.size())
                continue;

            Triangle tri;
            tri.v0 = vertices[vi0];
            tri.v1 = vertices[vi1];
            tri.v2 = vertices[vi2];

            if (hasFileNormals && fv0.vn > 0 && fv1.vn > 0 && fv2.vn > 0)
            {
                int ni0 = fv0.vn - 1;
                int ni1 = fv1.vn - 1;
                int ni2 = fv2.vn - 1;

                if (ni0 >= 0 && ni0 < (int)normals.size() &&
                    ni1 >= 0 && ni1 < (int)normals.size() &&
                    ni2 >= 0 && ni2 < (int)normals.size())
                {
                    tri.n0 = normals[ni0];
                    tri.n1 = normals[ni1];
                    tri.n2 = normals[ni2];
                    tri.hasVertexNormals = true;
                }
            }
            else if (!hasFileNormals)
            {
                tri.n0 = smoothNormals[vi0];
                tri.n1 = smoothNormals[vi1];
                tri.n2 = smoothNormals[vi2];
                tri.hasVertexNormals = true;
            }

            if (hasFileTexCoords && fv0.vt > 0 && fv1.vt > 0 && fv2.vt > 0)
            {
                int ti0 = fv0.vt - 1;
                int ti1 = fv1.vt - 1;
                int ti2 = fv2.vt - 1;

                if (ti0 >= 0 && ti0 < (int)texCoords.size() &&
                    ti1 >= 0 && ti1 < (int)texCoords.size() &&
                    ti2 >= 0 && ti2 < (int)texCoords.size())
                {
                    tri.uv0 = texCoords[ti0];
                    tri.uv1 = texCoords[ti1];
                    tri.uv2 = texCoords[ti2];
                    tri.hasUVs = true;
                }
            }

            result.triangles.push_back(tri);
        }
    }

    result.success = true;
    std::cerr << "Loaded OBJ: " << filename << " ("
              << vertices.size() << " vertices, "
              << result.triangles.size() << " triangles)\n";

    return result;
}

#endif // OBJ_LOADER_HPP
