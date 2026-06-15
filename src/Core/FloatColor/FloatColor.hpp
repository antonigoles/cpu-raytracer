#pragma once

#include <glm/common.hpp>
class FloatColor {
public:
    float red;
    float green;
    float blue;
    float alpha;

    FloatColor() : 
        red(0), green(0), blue(0), alpha(0)
    {};

    FloatColor(float r, float g, float b) :
        red(r), green(g), blue(b), alpha(0)
    {};

    FloatColor(float r, float g, float b, float a) :
        red(r), green(g), blue(b), alpha(a)
    {};

    FloatColor(float v) :
        red(v), green(v), blue(v), alpha(0)
    {};

    FloatColor operator+(const FloatColor& other) const;

    FloatColor operator-(const FloatColor& other) const;

    FloatColor operator*(const FloatColor& other) const;

    FloatColor operator*(float scalar) const;

    friend FloatColor operator*(float scalar, const FloatColor& color) {
        return color * scalar;
    };

    float strength() const;

    float max_component() const
    {
        return glm::max(glm::max(red, green), blue);
    };
};
