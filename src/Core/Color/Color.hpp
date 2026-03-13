#pragma once
#include <stdint.h>
#include <Core/FloatColor/FloatColor.hpp>

class Color {
public:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;

    static Color from_floats(float red, float green, float blue, float alpha);

    FloatColor as_floats() const;

    Color pow(float n);

    Color operator+(const Color& other) const;

    Color operator-(const Color& other) const;

    Color operator*(const Color& other) const;

    Color operator*(float scalar) const;

    static Color rasterize_from_float_color(const FloatColor& float_color);

    friend Color operator*(float scalar, const Color& color) {
        return color * scalar;
    }
};