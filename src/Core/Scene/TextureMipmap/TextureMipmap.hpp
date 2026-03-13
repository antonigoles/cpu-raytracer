#pragma once
#include <vector>
#include <stdint.h>
#include <Core/Color/Color.hpp>
#include <string>

class TextureMipmap
{
public:
    int width, height, channels;
    std::vector<uint8_t> data;

    void load(const std::string& filepath, uint32_t level);

    Color sample(float u, float v) const;
};