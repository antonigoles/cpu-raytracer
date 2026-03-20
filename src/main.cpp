#include <iostream>
#include <stdint.h>
#include <Infrastructure/Logger/Logger.hpp>
#include <Demo/Demo.hpp>

int main() {
    auto scene_0 = Demo::setup_cornell_box();
    // Demo::live_preview(std::move(scene_0));
    // Demo::render_picture(std::move(scene_0), "./scene_0.jpg");

    // auto scene_1 = Demo::setup_sponza();
    // Demo::render_picture(scene_1, "./scene_1.jpg");

    auto scene_2 = Demo::setup_breakfast_room();
    // Demo::live_preview(scene_2);
    Demo::render_picture(std::move(scene_2), "./scene_2_after_rework.jpg");

    return 0;
}