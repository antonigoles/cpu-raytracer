#include <Demo/Demo.hpp>
#include <Core/Scene/Scene.hpp>
#include <Infrastructure/ImageWriter/ImageWriter.hpp>
#include <Infrastructure/LivePreview/LivePreview.hpp>
#include <Core/Raytracing/Raytracing.hpp>
#include <Misc/Math/Math.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <Core/Scene/SceneLoader/SceneLoader.hpp>

void Demo::animation_test(std::shared_ptr<Scene> scene) {
    auto rt_engine = std::make_shared<EmbreeRayTracingEngine>();
    rt_engine->build_from_scene(scene);
    auto image_writer = ImageWriter();
    auto ray_tracer = BasicRayTracer(rt_engine);

    std::vector<Buffer2D<Fragment>> video_buffer;

    const uint32_t frame_count = 240;

    for (uint32_t i = 1; i <= frame_count; i++) {
        scene->camera->position.x += 0.05f;
        scene->camera->position.y += 0.15f;
        scene->camera->rotation = Math::EulerToQuatRadians(glm::radians(-(float)i / 3.0f), glm::radians(90.0f), 0.0f);

        Buffer2D<Fragment> ray_traced_buffer = ray_tracer.ray_trace_scene(scene, 512, 512);
        video_buffer.push_back(std::move(ray_traced_buffer));
        log_info("Frame ", i, " out of ", frame_count);
    }

    image_writer.write_vide_from_buffer_vector(video_buffer, "./output.mp4", 60);
}

void Demo::render_picture(std::shared_ptr<Scene> scene, std::string as, int sample_per_pixel, float jitter_scale) {
    auto rt_engine = std::make_shared<HavranKDTreeRayTracingEngine>();
    // auto rt_engine = std::make_shared<EmbreeRayTracingEngine>();
    rt_engine->build_from_scene(scene);
    auto image_writer = ImageWriter();
    auto ray_tracer = BasicRayTracer(rt_engine);

    auto ray_traced_buffer = ray_tracer.ray_trace_scene(scene, 1280, 720, sample_per_pixel, jitter_scale);
    image_writer.write_jpg_from_frame_buffer(&ray_traced_buffer, as);
}

void Demo::live_preview(std::shared_ptr<Scene> scene)
{
    auto main_camera = std::make_shared<Camera>();
    auto rt_engine = std::make_shared<EmbreeRayTracingEngine>();
    rt_engine->build_from_scene(scene);
    main_camera->up = glm::vec3(0.0f, 1.0f, 0.0f);
    scene->camera = main_camera;
    auto ray_tracer = BasicRayTracer(rt_engine);
    auto live_preview = LivePreview(1280, 720);

    live_preview.init_window();
    float delta_time = 0.01f;

    float camera_angle_sideways = glm::radians(90.0f);
    float camera_angle_upwards = glm::radians(-20.0f);

    main_camera->fov = glm::radians(45.0f);

    while (!live_preview.window_should_close()) {
        auto timestamp = glfwGetTime();
        auto ray_traced_buffer = ray_tracer.ray_trace_scene(
            scene, 
            1280, 
            720,
            1
        );
        live_preview.load_frame(ray_traced_buffer.bytes());
        delta_time = glfwGetTime() - timestamp;
        // log_info(1.0f / delta_time, " FPS");

        // Controls
        glm::vec3 move = glm::vec3(0.0f,0.0f,0.0f);

        float multiplier = live_preview.is_held(GLFW_KEY_LEFT_SHIFT) ? 2.0f : 1.0f;

        if (live_preview.is_held(GLFW_KEY_W)) move += scene->camera->get_forward();
        if (live_preview.is_held(GLFW_KEY_S)) move -= scene->camera->get_forward();
        if (live_preview.is_held(GLFW_KEY_A)) move -= scene->camera->get_right();
        if (live_preview.is_held(GLFW_KEY_D)) move += scene->camera->get_right();

        if (live_preview.is_held(GLFW_KEY_UP)) camera_angle_upwards += multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_DOWN)) camera_angle_upwards -= multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_LEFT)) camera_angle_sideways += multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_RIGHT)) camera_angle_sideways -= multiplier * delta_time;

        if (live_preview.is_held(GLFW_KEY_P)) {
            scene->sphere_light_sources[0].radius += delta_time;
            scene->sphere_light_sources[1].radius += delta_time;
        }

        if (live_preview.is_held(GLFW_KEY_O)) {
            scene->sphere_light_sources[0].radius -= delta_time;
            scene->sphere_light_sources[1].radius -= delta_time;
        }

        scene->camera->rotation = Math::EulerToQuatRadians(camera_angle_upwards, camera_angle_sideways, 0.0f);
        scene->camera->position += glm::length(move) == 0 ? move : glm::normalize(move) * delta_time * multiplier;

        log_info(
            "pos: ",
            scene->camera->position.x, " ", 
            scene->camera->position.y, " ", 
            scene->camera->position.z, 
            "rot: ",
            camera_angle_sideways,
            camera_angle_upwards
        );
    }
    live_preview.terminate();
}

std::unique_ptr<Scene> Demo::setup_sponza()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(10.0f, 8.8f, 0.0f);
    main_camera->rotation = Math::EulerToQuatRadians(glm::radians(0.0f), glm::radians(90.0f), 0.0f);
    main_camera->fov = glm::radians(70.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/Sponza/sponza.obj");

    scene->point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(-5.0f, 10.0f, 0.0f),
        .color = Color(34, 56, 255, 255).as_floats(),
        .strength = 500.0f
    });

    scene->point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(0.0f, 10.0f, 0.0f),
        .color = Color(21, 188, 42, 255).as_floats(),
        .strength = 500.0f
    });

    scene->point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(5.0f, 10.0f, 0.0f),
        .color = Color(156, 23, 24, 255).as_floats(),
        .strength = 500.0f
    });

    scene->camera = main_camera;

    return scene;
}

std::unique_ptr<Scene> Demo::setup_breakfast_room()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(-3.0f, 2.0f, 3.0f);
    main_camera->rotation = Math::EulerToQuatRadians(glm::radians(0.0f), glm::radians(-20.0f), 0.0f);
    main_camera->fov = glm::radians(70.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/breakfast_room/breakfast_room.obj");

    scene->sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(-2.0f, 4.0f, -2.0f),
        0.1f,
        Color(233, 166, 166, 255).as_floats(),
        5.0f
    ));

    scene->sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(1.0f, 4.0f, -2.0f),
        0.1f,
        Color(233, 166, 166, 255).as_floats(),
        5.0f
    ));

    scene->camera = main_camera;

    return scene;
}

std::unique_ptr<Scene> Demo::setup_cornell_box()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(0.0f, 0.8f, 2.0f);
    main_camera->rotation = Math::EulerToQuatRadians(glm::radians(0.0f), glm::radians(0.0f), 0.0f);
    main_camera->fov = glm::radians(60.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/CornellBox/CornellBox-Sphere.obj");
    scene->camera = main_camera;


    scene->sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(0.0f, 1.5f, 0.0f),
        0.1f,
        Color(255, 255, 255, 255).as_floats(),
        1.0f
    ));

    return scene;
}