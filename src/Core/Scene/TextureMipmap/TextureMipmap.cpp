#include <Core/Scene/TextureMipmap/TextureMipmap.hpp>
#include <stb/stb.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <glm/glm.hpp>

void TextureMipmap::load(const std::string& filepath, uint32_t level)
{
    uint8_t* img = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
    if (!img) {
        log_err("Could not load texture: ", filepath);
        exit(-1);
    }

    uint32_t divisor = glm::pow(2, level);

    uint32_t new_width = width / divisor;
    uint32_t new_height = height / divisor;

    uint8_t* resized_pixels = (uint8_t*)malloc(new_width * new_height * 4);
    stbir_resize_uint8(img, width, height, 0, resized_pixels, new_width, new_height, 0, 4);
    stbi_image_free(img);
    data.assign(resized_pixels, (resized_pixels + new_height * new_width * 4));

    width = new_width;
    height = new_height;
    free(resized_pixels);
}

Color TextureMipmap::sample(float u, float v) const {
    if (data.empty()) return Color::from_floats(1.0f, 0.0f, 1.0f, 1.0f);

    u = u - std::floor(u);
    v = v - std::floor(v);

    int x = (int)(u * width) % width;
    
    int y = (int)((1.0f - v) * height) % height; 

    int index = (y * width + x) * 4;

    return Color{
        .red   = data[index + 0],
        .green = data[index + 1],
        .blue  = data[index + 2],
        .alpha = data[index + 3]
    };
}
