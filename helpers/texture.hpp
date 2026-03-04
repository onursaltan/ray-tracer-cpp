#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "math.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <cmath>

class Texture
{
private:
    int width, height;
    std::vector<Vec3> data;

public:
    Texture() : width(0), height(0) {}

    Texture(int w, int h) : width(w), height(h)
    {
        data.resize(w * h, Vec3(1, 1, 1));
    }

    Vec3 sample(float u, float v) const
    {
        if (data.empty())
            return Vec3(1, 1, 1);

        u = u - std::floor(u);
        v = v - std::floor(v);

        int x = (int)(u * width) % width;
        int y = (int)(v * height) % height;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x >= width)
            x = width - 1;
        if (y >= height)
            y = height - 1;

        return data[y * width + x];
    }

    Vec3 sampleBilinear(float u, float v) const
    {
        if (data.empty())
            return Vec3(1, 1, 1);

        u = u - std::floor(u);
        v = v - std::floor(v);

        float x = u * width - 0.5f;
        float y = v * height - 0.5f;

        int x0 = (int)std::floor(x);
        int y0 = (int)std::floor(y);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        float fx = x - x0;
        float fy = y - y0;

        x0 = (x0 % width + width) % width;
        y0 = (y0 % height + height) % height;
        x1 = (x1 % width + width) % width;
        y1 = (y1 % height + height) % height;

        Vec3 c00 = data[y0 * width + x0];
        Vec3 c10 = data[y0 * width + x1];
        Vec3 c01 = data[y1 * width + x0];
        Vec3 c11 = data[y1 * width + x1];

        Vec3 c0 = c00 * (1 - fx) + c10 * fx;
        Vec3 c1 = c01 * (1 - fx) + c11 * fx;
        return c0 * (1 - fy) + c1 * fy;
    }

    bool loadImage(const std::string &filename)
    {
        if (filename.find(".ppm") != std::string::npos)
        {
            return loadPPM(filename);
        }

        int channels;
        unsigned char *imageData = stbi_load(filename.c_str(), &width, &height, &channels, 3);

        if (!imageData)
        {
            std::cerr << "Failed to load texture: " << filename << " - " << stbi_failure_reason() << "\n";
            return false;
        }

        data.resize(width * height);
        for (int i = 0; i < width * height; i++)
        {
            data[i] = Vec3(
                imageData[i * 3 + 0] / 255.0f,
                imageData[i * 3 + 1] / 255.0f,
                imageData[i * 3 + 2] / 255.0f);
        }

        stbi_image_free(imageData);
        std::cerr << "Loaded texture: " << filename << " (" << width << "x" << height << ")\n";
        return true;
    }

    bool loadPPM(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open texture: " << filename << "\n";
            return false;
        }

        std::string format;
        file >> format;

        if (format != "P6")
        {
            std::cerr << "Unsupported PPM format: " << format << "\n";
            return false;
        }

        file >> width >> height;
        int maxVal;
        file >> maxVal;
        file.get();

        data.resize(width * height);

        for (int i = 0; i < width * height; i++)
        {
            unsigned char rgb[3];
            file.read((char *)rgb, 3);
            data[i] = Vec3(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f);
        }

        file.close();
        std::cerr << "Loaded texture: " << filename << " (" << width << "x" << height << ")\n";
        return true;
    }

    static Texture createCheckerboard(int size, const Vec3 &color1, const Vec3 &color2)
    {
        Texture tex(size, size);
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                bool isEven = ((x / 16) + (y / 16)) % 2 == 0;
                tex.data[y * size + x] = isEven ? color1 : color2;
            }
        }
        return tex;
    }

    static Texture createTestNormalMap(int size)
    {
        Texture tex(size, size);
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float waveX = std::sin(x * 0.1f) * 0.5f;
                float waveY = std::sin(y * 0.1f) * 0.5f;

                tex.data[y * size + x] = Vec3(
                    0.5f + waveX * 0.3f,
                    0.5f + waveY * 0.3f,
                    0.8f);
            }
        }
        return tex;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

class TextureManager
{
private:
    std::map<std::string, Texture> textures;

public:
    static TextureManager &getInstance()
    {
        static TextureManager instance;
        return instance;
    }

    bool loadTexture(const std::string &name, const std::string &filename)
    {
        Texture tex;
        if (tex.loadImage(filename))
        {
            textures[name] = tex;
            return true;
        }
        return false;
    }

    void addProceduralTexture(const std::string &name, const Texture &tex)
    {
        textures[name] = tex;
    }

    const Texture *getTexture(const std::string &name) const
    {
        auto it = textures.find(name);
        if (it != textures.end())
            return &it->second;
        return nullptr;
    }

    bool hasTexture(const std::string &name) const
    {
        return textures.find(name) != textures.end();
    }
};

#endif
