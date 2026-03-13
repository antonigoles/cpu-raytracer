#include <Core/FloatColor/FloatColor.hpp>

FloatColor FloatColor::operator+(const FloatColor& other) const {
    return FloatColor{
        .red   = red + other.red,
        .green = green + other.green,
        .blue  = blue + other.blue,
        .alpha = alpha + other.alpha
    };
}

FloatColor FloatColor::operator-(const FloatColor& other) const {
    return FloatColor{
        .red   = red - other.red,
        .green = green - other.green,
        .blue  = blue - other.blue,
        .alpha = alpha - other.alpha
    };
}

FloatColor FloatColor::operator*(const FloatColor& other) const {
    return FloatColor{
        .red   = red * other.red,
        .green = green * other.green,
        .blue  = blue * other.blue,
        .alpha = alpha * other.alpha
    };
}

FloatColor FloatColor::operator*(float scalar) const {
    return FloatColor{
        .red   = red * scalar,
        .green = green * scalar,
        .blue  = blue * scalar,
        .alpha = alpha * scalar
    };
}