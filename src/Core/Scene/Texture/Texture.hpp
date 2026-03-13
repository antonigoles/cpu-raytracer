#pragma once
#include <vector>
#include <string>
#include <Core/Scene/TextureMipmap/TextureMipmap.hpp>
#include <Core/Color/Color.hpp>

class Texture {
public:
    bool is_empty;
    std::vector<TextureMipmap> mipmaps;

    Texture();

    Texture(const std::string& filepath);

    Color sample(float u, float v, float distance) const;
};