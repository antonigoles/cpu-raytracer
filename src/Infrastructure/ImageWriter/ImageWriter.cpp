#include <Infrastructure/ImageWriter/ImageWriter.hpp>
#include <stb/stb.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgba.h>
#include <cmath>

ImageWriter::ImageWriter() {};

void ImageWriter::write_jpg_from_frame_buffer(Buffer2D<Fragment> *buffer, const std::string& path) {
    log_info("Writing image buffer to ", path);
    stbi_write_jpg(
        path.c_str(), 
        buffer->get_width(), 
        buffer->get_height(), 
        4, 
        buffer->bytes(), 
        buffer->get_width() * 4
    );
}

void ImageWriter::write_vide_from_buffer_vector(std::vector<Buffer2D<Fragment>>& frames, const std::string& filepath, int fps) 
{
    if (frames.size() == 0) {
        log_err("Empty video buffer passed to write method");
        exit(-1);
    }
    uint32_t width = frames[0].get_width();
    uint32_t height = frames[0].get_height();


    std::string cmd = "ffmpeg -v warning -y -f rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + 
                    " -pix_fmt rgb24 -r " + std::to_string(fps) + 
                    " -i - -c:v libx264 -preset fast -pix_fmt yuv420p \"" + filepath + "\"";

    FILE* ffmpeg = popen(cmd.c_str(), "w");
    if (!ffmpeg) {
        log_err("FFMpeg was not found. Could not save video");
        return;
    }

    std::vector<uint8_t> rgb_buffer(width * height * 3);

    for (size_t i = 0; i < frames.size(); ++i) 
    {
        Buffer2D<Fragment>& frame = frames[i];

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                Fragment* frag = frame.at(x, y);
                
                int index = (y * width + x) * 3;
                rgb_buffer[index + 0] = frag->red;
                rgb_buffer[index + 1] = frag->green;
                rgb_buffer[index + 2] = frag->blue;
            }
        }

        fwrite(rgb_buffer.data(), 1, rgb_buffer.size(), ffmpeg);
    }

    pclose(ffmpeg);
    
    log_info("Video saved as ", filepath);
}

bool check_illegal_hdr_float(float value)
{
    return std::isnan(value) || std::isinf(value) || value < 0.0f;
}

bool check_illegal_hdr_color(FloatColor* value)
{
    return check_illegal_hdr_float(value->red) ||
        check_illegal_hdr_float(value->green) ||
        check_illegal_hdr_float(value->blue) ||
        check_illegal_hdr_float(value->alpha);
}

void ImageWriter::write_exr_from_floatcolor_buffer(Buffer2D<FloatColor> *buffer, const std::string& path)
{
    int width = buffer->get_width();
    int height = buffer->get_height();
    Imf::Array2D<Imf::Rgba> pixels(height, width);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            FloatColor* col = buffer->at(x, y);
            pixels[y][x] = Imf::Rgba(
                std::min(65000.0f, col->red), 
                std::min(65000.0f, col->green), 
                std::min(65000.0f, col->blue), 
                1.0f
            );
            if (check_illegal_hdr_color(col)) {
                log_info("Illegal value spotted during EXR compilation");
            }
        }
    }

    try {
        Imf::RgbaOutputFile file(path.c_str(), width, height, Imf::WRITE_RGBA);
        file.setFrameBuffer(&pixels[0][0], 1, width);
        file.writePixels(height);
        log_info("EXR Image saved to: ", path);
    } catch (const std::exception& e) {
        log_err("Error while saving to OpenEXR: ", e.what());
    }
}