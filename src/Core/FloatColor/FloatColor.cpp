#include <Core/FloatColor/FloatColor.hpp>
#include <glm/glm.hpp>

FloatColor FloatColor::operator+(const FloatColor& other) const {
    return FloatColor(red + other.red, green + other.green, blue + other.blue, alpha + other.alpha);
}

FloatColor FloatColor::operator-(const FloatColor& other) const {
    return FloatColor(red - other.red, green - other.green, blue - other.blue, alpha - other.alpha);
}

FloatColor FloatColor::operator*(const FloatColor& other) const {
    return FloatColor(red * other.red, green * other.green, blue * other.blue, alpha * other.alpha);
}

FloatColor FloatColor::operator*(float scalar) const {
    return FloatColor(red * scalar, green * scalar, blue * scalar, alpha * scalar);
}

/**
    Luminance from RGB
*/
float FloatColor::strength() const {
    return 0.2126f * this->red + 0.7152f * this->green + 0.0722f * this->blue;
}