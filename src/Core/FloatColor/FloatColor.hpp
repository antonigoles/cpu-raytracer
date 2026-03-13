#pragma once

class FloatColor {
public:
    float red;
    float green;
    float blue;
    float alpha;

    FloatColor operator+(const FloatColor& other) const;

    FloatColor operator-(const FloatColor& other) const;

    FloatColor operator*(const FloatColor& other) const;

    FloatColor operator*(float scalar) const;

    friend FloatColor operator*(float scalar, const FloatColor& color) {
        return color * scalar;
    }
};
