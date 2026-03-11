#pragma once
#include <stdint.h>
#include <GLFW/glfw3.h>

class LivePreview
{
private:
    GLuint texture_id;
    uint32_t width, height;
    GLFWwindow* window;
public:
    LivePreview(uint32_t width, uint32_t height);

    bool window_should_close();
    void terminate();

    bool is_held(uint32_t key);
    bool is_clicked_once(uint32_t key);

    void init_window();
    void load_frame(uint8_t *data);
};