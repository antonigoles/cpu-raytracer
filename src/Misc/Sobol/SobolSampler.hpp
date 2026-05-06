#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <memory>
#include <utility>

class SobolSampler {
private:
    uint32_t current_sample_index;
    uint32_t current_dimension;
    uint32_t pixel_seed;
    uint32_t pixel_hash;

    inline uint32_t pcg_hash(uint32_t input) const {
        uint32_t state = input * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }

    inline uint32_t hash_function(uint32_t x, uint32_t y) const {
        return pcg_hash(x ^ pcg_hash(y));
    }

    inline float random_float_from_hash() {
        pixel_seed = pixel_seed * 1664525u + 1013904223u;
        return (float)(pixel_seed & 0x00FFFFFF) / (float)0x01000000;
    }

    inline float generate_sobol(uint32_t index, uint32_t dimension) const {
        static const uint32_t direction_numbers[3][32] = {
            { 0x80000000, 0x40000000, 0x20000000, 0x10000000, 0x8000000, 0x4000000, 0x2000000, 0x1000000, 0x800000, 0x400000, 0x200000, 0x100000, 0x80000, 0x40000, 0x20000, 0x10000, 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 },
            { 0x80000000, 0xc0000000, 0xa0000000, 0xf0000000, 0x8800000, 0xcc00000, 0xaa00000, 0xff00000, 0x808000, 0xc0c000, 0xa0a000, 0xf0f000, 0x80080, 0xc00c0, 0xa00a0, 0xf00f0, 0x80008, 0xc000c, 0xa000a, 0xf000f, 0x80800, 0xc0c00, 0xa0a00, 0xf0f00, 0x80, 0xc0, 0xa0, 0xf0, 0x8, 0xc, 0xa, 0xf },
            { 0x80000000, 0xc0000000, 0x60000000, 0x50000000, 0xa800000, 0xcc00000, 0x6600000, 0x5500000, 0x8a8000, 0xccc000, 0x666000, 0x555000, 0x80a80, 0xc0cc0, 0x60660, 0x50550, 0x800a8, 0xc00cc, 0x60066, 0x50055, 0x80a80, 0xc0cc0, 0x60660, 0x50550, 0x8a, 0xcc, 0x66, 0x55, 0x8, 0xc, 0x6, 0x5 }
        };

        uint32_t safe_dim = dimension % 3; 
        uint32_t result = 0;
        uint32_t i = 0;
        uint32_t temp_index = index;

        while (temp_index != 0) {
            if (temp_index & 1) {
                result ^= direction_numbers[safe_dim][i];
            }
            temp_index >>= 1;
            i++;
        }

        return (float)result / 4294967296.0f;
    }

public:
    SobolSampler() : current_sample_index(0), current_dimension(0), pixel_seed(0) {}

    void start_sample(uint32_t sample_index, uint32_t x, uint32_t y) {
        current_sample_index = sample_index;
        current_dimension = 0;
        pixel_hash = hash_function(x, y); 
    }

    float get_1d() {
        float sobol_value = generate_sobol(current_sample_index, current_dimension);
        current_dimension++;
        float scrambled = sobol_value + random_float_from_hash();
        
        if (scrambled >= 1.0f) {
            scrambled -= 1.0f;
        }

        return scrambled;
    }

    glm::vec2 get_2d() {
        return glm::vec2(get_1d(), get_1d());
    }
};