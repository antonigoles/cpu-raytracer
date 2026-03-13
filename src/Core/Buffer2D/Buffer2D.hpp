#pragma once
#include <stdint.h>

template<typename T>
class Buffer2D {
private:
    T* buffer_data;
    uint32_t width;
    uint32_t height;

public:
    Buffer2D(uint32_t width, uint32_t height) : width(width), height(height) {
        buffer_data = new T[width * height];
    };

    ~Buffer2D() {
        delete[] buffer_data;
    };

    Buffer2D(Buffer2D&& other) noexcept : width(other.width), height(other.height), buffer_data(other.buffer_data) {
        other.buffer_data = nullptr; 
        other.width = 0;
        other.height = 0;
    };

    Buffer2D(const Buffer2D&) = delete;
    Buffer2D& operator=(const Buffer2D&) = delete;

    uint32_t get_width() {
        return width;
    };

    uint32_t get_height() {
        return height;
    };

    T* at(uint32_t x, uint32_t y) {
        return &this->buffer_data[this->width * y + x];
    };

    void write(uint32_t x, uint32_t y, T data) {
        this->buffer_data[this->width * y + x] = data;
    };

    uint8_t* bytes() {
        return (uint8_t*)buffer_data;
    };
};