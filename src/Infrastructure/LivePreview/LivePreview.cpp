#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <Infrastructure/LivePreview/LivePreview.hpp>
#include <unordered_set>

LivePreview::LivePreview(uint32_t width, uint32_t height) : width(width), height(height) {};

void LivePreview::init_window() {
    if (!glfwInit()) {
        std::cerr << "Nie udalo sie zainicjowac GLFW!" << std::endl;
        return;
    }

    window = glfwCreateWindow(this->width, this->height, "RayTracer Live Preview", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glEnable(GL_TEXTURE_2D);
};

void LivePreview::load_frame(uint8_t *data) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
};

bool LivePreview::window_should_close() {
    return glfwWindowShouldClose(window);
}

void LivePreview::terminate() {
    glfwTerminate();
};

bool LivePreview::is_held(uint32_t key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
};

bool LivePreview::is_clicked_once(uint32_t key) {
    static std::unordered_set<uint32_t> key_lock;
    if (key_lock.contains(key)) {
        if (glfwGetKey(window, key) == GLFW_RELEASE) {
            key_lock.erase(key);
        }
        return false;
    }

    if (glfwGetKey(window, key) == GLFW_PRESS) {
        key_lock.insert(key);
        return true;
    }

    return false;
};