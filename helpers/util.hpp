#include <cstdint>
#include <fstream>
#include <iostream>
#include "math.hpp"
#include "primitives.hpp"
#include "../objects/cube.hpp"

bool createPPM(const char *filename, int width, int height, std::ofstream &out)
{
    out.open(filename, std::ios::binary);
    if (!out)
    {
        std::cerr << "failed to open " << filename << "\n";
        return false;
    }
    out << "P6\n"
        << width << " " << height << "\n255\n";
    return true;
}