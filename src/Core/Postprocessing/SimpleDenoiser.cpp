#include <Core/Postprocessing/SimpleDenoisser.hpp>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

std::vector<FloatColor*> gather_neighbors(
    Buffer2D<FloatColor>& buffer, 
    int32_t x, 
    int32_t y, 
    int32_t size
) {
    std::vector<FloatColor*> colors;
    colors.reserve((size * 2 + 1) * (size * 2 + 1));
    for (int32_t dx = -size; dx <= size; dx++ ) {
        int32_t xprim = x + dx;
        if (xprim < 0 || xprim >= buffer.get_width()) continue;
        for (int32_t dy = -size; dy <= size; dy++ ) {
            int32_t yprim = y + dy;
            if (yprim < 0 || yprim >= buffer.get_height()) continue;

            colors.push_back(buffer.at(xprim, yprim));
        }
    }
    return colors;
}

FloatColor* get_neighbourhood_median(
    Buffer2D<FloatColor>& buffer, 
    uint32_t x, 
    uint32_t y, 
    uint32_t size
) {
    std::vector<FloatColor*> colors = gather_neighbors(buffer, x, y, size);
    std::sort(colors.begin(), colors.end(), [](const FloatColor* a, FloatColor* b) {
        return a->strength() < b->strength();
    });
    return colors[colors.size() / 2];
}

struct DenoiseUpdate
{
    uint32_t x;
    uint32_t y;
    float red;
    float green;
    float blue; 
};

void SimpleDenoiser::inplace_denoise(Buffer2D<FloatColor>& buffer)
{
    std::vector<DenoiseUpdate> updates;

    for (uint32_t x = 0; x < buffer.get_width(); x++) {
        for (uint32_t y = 0; y < buffer.get_height(); y++) {
            FloatColor* median = get_neighbourhood_median(buffer, x, y, 2);
            float current = buffer.at(x, y)->strength();
            if (current / (0.0001f + median->strength()) > 10) {
                updates.push_back(DenoiseUpdate{
                    x, y, median->red, median->green, median->blue
                });
            }
        }
    }

    for (auto& update : updates) {
        buffer.at(update.x, update.y)->red = update.red;
        buffer.at(update.x, update.y)->green = update.green;
        buffer.at(update.x, update.y)->blue = update.blue;
    }
};
