#pragma once
#include <Core/Buffer2D/Buffer2D.hpp>
#include <Core/Fragment/Fragment.hpp>
#include <string>
#include <vector>

class ImageWriter
{
public:
    ImageWriter();

    void write_jpg_from_frame_buffer(Buffer2D<Fragment> *buffer, const std::string& path);

    void write_vide_from_buffer_vector(std::vector<Buffer2D<Fragment>>& frames, const std::string& filepath, int fps = 60);
};