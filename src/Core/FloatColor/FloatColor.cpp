#include <Core/FloatColor/FloatColor.hpp>
#include <glm/glm.hpp>

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

float FloatColor::strength() const {
    // tak podobno działa Fizyka - to jest luminance from RGB
    return 0.2126f * this->red + 0.7152f * this->green + 0.0722f * this->blue;
}