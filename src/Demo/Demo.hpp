#pragma once
#include <Core/Scene/Scene.hpp>

class Demo
{   
public:
    static void animation_test(Scene& scene);

    static void render_picture(const Scene& scene, std::string as = "./output.jpg", int sample_per_pixel = 1, float jitter_scale = (0.25F));

    static void live_preview(Scene& scene);

    static Scene setup_sponza();

    static Scene setup_breakfast_room();

    static Scene setup_cornell_box();
};