#include <Core/Color/Color.hpp>
#include <glm/glm.hpp>

Color Color::from_floats(float red, float green, float blue, float alpha)
{
    return Color{
        .red = (uint8_t)(red >= 1.0f ? 255 : red * 255.0f),
        .green = (uint8_t)(green >= 1.0f ? 255 : green * 255.0f),
        .blue = (uint8_t)(blue >= 1.0f ? 255 : blue * 255.0f),
        .alpha = (uint8_t)(alpha >= 1.0f ? 255 : alpha * 255.0f),
    };
}

FloatColor Color::as_floats() const {
    return FloatColor{ (float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f };
}

Color Color::pow(float n)
{
    auto v = this->as_floats();
    return Color::from_floats(
        glm::pow(v.red, n),
        glm::pow(v.green, n),
        glm::pow(v.blue, n),
        glm::pow(v.alpha, n)
    );
}

Color Color::operator+(const Color& other) const {
    return Color{
        .red   = static_cast<uint8_t>(glm::clamp(red + other.red, 0, 255)),
        .green = static_cast<uint8_t>(glm::clamp(green + other.green, 0, 255)),
        .blue  = static_cast<uint8_t>(glm::clamp(blue + other.blue, 0, 255)),
        .alpha = static_cast<uint8_t>(glm::clamp(alpha + other.alpha, 0, 255))
    };
}

Color Color::operator-(const Color& other) const {
    return Color{
        .red   = static_cast<uint8_t>(glm::clamp(red - other.red, 0, 255)),
        .green = static_cast<uint8_t>(glm::clamp(green - other.green, 0, 255)),
        .blue  = static_cast<uint8_t>(glm::clamp(blue - other.blue, 0, 255)),
        .alpha = static_cast<uint8_t>(glm::clamp(alpha - other.alpha, 0, 255))
    };
}

Color Color::operator*(const Color& other) const {
    auto mult = this->as_floats() * other.as_floats();
    return Color::from_floats(mult.red, mult.green, mult.blue, mult.alpha);
}

Color Color::operator*(float scalar) const {
    return Color{
        .red   = static_cast<uint8_t>(glm::clamp(red * scalar, 0.0f, 255.0f)),
        .green = static_cast<uint8_t>(glm::clamp(green * scalar, 0.0f, 255.0f)),
        .blue  = static_cast<uint8_t>(glm::clamp(blue * scalar, 0.0f, 255.0f)),
        .alpha = static_cast<uint8_t>(glm::clamp(alpha * scalar, 0.0f, 255.0f))
    };
}


Color Color::rasterize_from_float_color(const FloatColor& float_color)
{
    return Color::from_floats(
        glm::pow(float_color.red, 1.0f / 2.2f),
        glm::pow(float_color.green, 1.0f / 2.2f),
        glm::pow(float_color.blue, 1.0f / 2.2f), 
        glm::pow(float_color.alpha, 1.0f / 2.2f)
    );
}