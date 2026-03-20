#pragma once
#include <Core/Scene/Scene.hpp>

class Demo
{   
public:
    static void animation_test(std::shared_ptr<Scene> scene);

    static void render_picture(std::shared_ptr<Scene> scene, std::string as = "./output.jpg", int sample_per_pixel = 1, float jitter_scale = (0.25F));

    static void live_preview(std::shared_ptr<Scene> scene);

    static std::unique_ptr<Scene> setup_sponza();

    static std::unique_ptr<Scene> setup_breakfast_room();

    static std::unique_ptr<Scene> setup_cornell_box();
};