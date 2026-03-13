#include <Core/Scene/Texture/Texture.hpp>
#include <glm/glm.hpp>

Texture::Texture() {
    // empty texture
    mipmaps = {};
    is_empty = true;
}

Texture::Texture(const std::string& filepath) {
    for (int i = 0; i <= 4; i++) {
        TextureMipmap mipmap;
        mipmap.load(filepath, i);
        this->mipmaps.push_back(std::move(mipmap));
    }
    is_empty = false;
}

Color Texture::sample(float u, float v, float distance) const {
    if (mipmaps.empty()) return Color(0.0f, 0.0f, 0.0f, 0.0f);
    uint32_t mipmap_idx = glm::clamp((int)(distance / 6.0), 0, (int)mipmaps.size() - 1);
    return mipmaps[mipmap_idx].sample(u, v);
}