#include <iostream>
#include <stdint.h>
#include <Infrastructure/Logger/Logger.hpp>
#include <Demo/Demo.hpp>

int main() {
    auto scene_0 = Demo::setup_cornell_box();
    scene_0.build_embree();
    Demo::live_preview(scene_0);
    // Demo::render_picture(scene_0, "./scene_0.jpg", 1, 0.0005f);

    // auto scene_1 = Demo::setup_sponza();
    // scene_1.build_embree();
    // Demo::render_picture(scene_1, "./scene_1.jpg");

    // auto scene_2 = Demo::setup_breakfast_room();
    // scene_2.build_embree();
    // Demo::live_preview(scene_2);
    // Demo::render_picture(scene_2, "./scene_2_on_floats.jpg");

    return 0;
}