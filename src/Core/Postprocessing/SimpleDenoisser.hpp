#pragma once
#include <Core/Buffer2D/Buffer2D.hpp>
#include <Core/FloatColor/FloatColor.hpp>

class SimpleDenoiser {
public:
    static void inplace_denoise(Buffer2D<FloatColor>& buffer);
};