#include <iostream>
#include <stdint.h>
#include <Infrastructure/Logger/Logger.hpp>
#include <Demo/Demo.hpp>
#include <boost/program_options.hpp>
#include <Core/Scene/SceneLoader/SceneLoader.hpp>
#include <Misc/Math/Math.hpp>
#include <Core/Raytracing/Raytracing.hpp>
#include <Infrastructure/ImageWriter/ImageWriter.hpp>
#include <chrono>
#include <Core/Postprocessing/SimpleDenoisser.hpp>

struct CameraConfig {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fovy = 45.0f;
    float focal_length = 1.0f;
};

struct SceneConfig {
    CameraConfig cam;
    std::vector<PointLightSource> point_lights;
    std::vector<SphereLightSource> sphere_lights;
    int recursion_level = 0;
    glm::ivec2 resolution{800, 600};
    std::string output_file = "out.jpg";
    std::string input_path = "";
    std::string engine = "Embree";
    std::string metrics_path = "metrics.log";
    float ray_normal_bias = 0.0001f;
    bool live_preview = false;
    int sample_per_pixel_sqrt = 4;
    float jitter_scale = 0.25f;
    int nl_parameter = 1;
    bool write_exr = false;
    int np = 0;
};

std::string v3_to_str(const glm::vec3 v) {
    return std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z);
}

void run_scene_form_config(const SceneConfig& config)
{
    // debug

    log_verbose(
        "cam.position: ", v3_to_str(config.cam.position), "\n",
        "cam.direction: ", v3_to_str(config.cam.direction), "\n",
        "cam.up: ", v3_to_str(config.cam.up), "\n",
        "cam.fovy: ", config.cam.fovy, "\n",
        "cam.focal_length: ", config.cam.focal_length, "\n",
        "point_lights.count(): ", config.point_lights.size(), "\n",
        "recursion_level: ", config.recursion_level, "\n",
        "resolution.x: ", config.resolution.x, "\n",
        "resolution.y: ", config.resolution.y, "\n",
        "output_file: ", config.output_file, "\n",
        "input_path: ", config.input_path, "\n",
        "engine: ", config.engine, "\n",
        "metrics_path: ", config.metrics_path, "\n",
        "ray_normal_bias: ", config.ray_normal_bias, "\n",
        "nl_parameter: ", config.nl_parameter, "\n",
        "np: ", config.np, "\n"
    );

    // setup the scene itself 
    auto main_camera = std::make_shared<Camera>();
    main_camera->up = config.cam.up;
    main_camera->position = config.cam.position;
    main_camera->rotation = glm::lookAt(config.cam.position, config.cam.position + config.cam.direction, config.cam.up);
    main_camera->fov = glm::radians(config.cam.fovy);
    auto scene_loader = SceneLoader();
    std::shared_ptr<Scene> scene = scene_loader.load_scene_from_file(config.input_path);
    scene->camera = main_camera;

    scene->point_light_sources = config.point_lights;
    scene->sphere_light_sources = config.sphere_lights;
    
    // Render scene
    std::shared_ptr<AbstractRayTracingEngine> rt_engine = std::make_shared<HavranKDTreeRayTracingEngine>();
    if (config.engine == "Embree") {
        rt_engine = std::make_shared<EmbreeRayTracingEngine>();
    }

    if (config.engine == "HavranKDTree") {
        rt_engine = std::make_shared<HavranKDTreeRayTracingEngine>();
    }

    // auto rt_engine = std::make_shared<EmbreeRayTracingEngine>();
    auto building_scene_start = std::chrono::high_resolution_clock::now();
    rt_engine->build_from_scene(scene);
    auto building_scene_end = std::chrono::high_resolution_clock::now();
    auto building_scene_duration = (double)std::chrono::duration_cast<std::chrono::milliseconds>(building_scene_end - building_scene_start).count() / 1000.0f;

    auto image_writer = ImageWriter();
    auto ray_tracer = BasicRayTracer(rt_engine);

    auto rendering_start = std::chrono::high_resolution_clock::now();
    uint32_t sample_count = config.sample_per_pixel_sqrt;
    if (config.np != 0) {
        sample_count = (uint32_t)glm::floor(glm::sqrt(config.np));
    }
    auto ray_traced_buffer = ray_tracer.ray_trace_scene_hdr(
        scene, 
        config.resolution.x, 
        config.resolution.y, 
        config.recursion_level,
        0.0001f,
        sample_count,
        config.jitter_scale,
        config.cam.focal_length,
        config.nl_parameter
    );
    auto rendering_end = std::chrono::high_resolution_clock::now();
    auto rendering_duration = (double)std::chrono::duration_cast<std::chrono::milliseconds>(rendering_end - rendering_start).count() / 1000.0f;
    
    if (config.write_exr) {
        log_info("Denoising...");
        SimpleDenoiser::inplace_denoise(ray_traced_buffer);
        image_writer.write_exr_from_floatcolor_buffer(&ray_traced_buffer, config.output_file);
    } else {
        log_err("NOT IMPLEMENTED ERROR: JPEG WRITING DOESNT WORK FOR NOW!");
    }

    float rays_per_second = (float)rt_engine->get_performance_metric().rays_shot / rendering_duration;
    
    log_file(config.metrics_path,
        "engine: ", config.engine, "\n",
        "rsa_structure_build_duration: ", building_scene_duration, "\n",
        "rendering_duration: ", rendering_duration, "\n",
        "total_rays_shot: ", rt_engine->get_performance_metric().rays_shot, "\n",
        "rays_per_second: ", rays_per_second
    );
}

glm::vec3 to_vec3(const std::vector<float>& v, const std::string& name) {
    if (v.size() != 3) {
        throw std::runtime_error("Argument " + name + " musi mieć dokładnie 3 wartości (x y z)!");
    }
    return glm::vec3(v[0], v[1], v[2]);
}

int boot_from_params(int argc, char **argv)
{
    SceneConfig config;

    namespace po = boost::program_options;
    po::options_description desc("Opcje renderera");
    desc.add_options()
            ("help,h", "Wyświetl pomoc")
            ("vp", po::value<std::vector<float>>()->multitoken(), "Pozycja kamery (x y z)")
            ("vd", po::value<std::vector<float>>()->multitoken(), "Kierunek patrzenia (x y z)")
            ("up", po::value<std::vector<float>>()->multitoken(), "Kierunek do góry (x y z)")
            
            ("np", po::value<int>(&config.np), "sample_per_pixel_sqrt do kwadratu")
            ("exr", po::value<bool>(&config.write_exr), "zapisz plik jako exr")
            ("nl", po::value<int>(&config.nl_parameter), "parametr nl")
            ("jitter_scale", po::value<float>(&config.jitter_scale), "jitter_scale")
            ("sample_per_pixel_sqrt", po::value<int>(&config.sample_per_pixel_sqrt), "sample_per_pixel_sqrt")
            ("focal_length", po::value<float>(&config.cam.focal_length), "Długość obiektywu")
            ("live_preview", po::value<bool>(&config.live_preview), "Uruchom w trybie real time")
            ("fovy", po::value<float>(&config.cam.fovy), "Kąt widzenia pionowy (stopnie)")
            ("r", po::value<int>(&config.recursion_level)->default_value(2), "Poziom rekurencji (0, 1, 2...)")
            ("o", po::value<std::string>(&config.output_file), "Nazwa pliku wynikowego")
            ("in", po::value<std::string>(&config.input_path), "Sciezka do pliku sceny obj")
            ("engine", po::value<std::string>(&config.engine)->default_value("Embree"), "Silnik RSA, dostępne: Embree, HavranKDTree")
            ("metrics_path", po::value<std::string>(&config.metrics_path), "Sciezka gdzie maja zostac zapisane metrics")
            ("ray_normal_bias", po::value<float>(&config.ray_normal_bias), 
            "Współczynnik przesuniecia promienia wzdłóż wektora normalnego odbijanego trójąta (do naprawiania shadow acnee)")

            ("res", po::value<std::vector<int>>()->multitoken(), "Rozdzielczość (dimx dimy)")
            
            ("point_lights", po::value<std::vector<float>>()->multitoken(), 
             "Point Lights: x_1 y_1 z_1 r_1 g_1 b_1 strength_1 ... x_n y_n z_n r_n g_n b_n strength_n  (wielokrotność 7 parametrów)")
            ("sphere_lights", po::value<std::vector<float>>()->multitoken(), 
             "Sphere Lights: x_1 y_1 z_1 r_1 g_1 b_1 strength_1 radius_1 ... x_n y_n z_n r_n g_n b_n strength_n radius_n (wielokrotność 8 parametrów)");


    po::variables_map vm;
    try {
        using namespace po::command_line_style;
        int style = allow_long | long_allow_adjacent | long_allow_next | case_insensitive | allow_long_disguise;

        po::store(
            po::parse_command_line(argc, argv, desc, style), 
            vm
        );

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        if (vm.count("vp")) config.cam.position = to_vec3(vm["vp"].as<std::vector<float>>(), "vp");
        if (vm.count("vd")) config.cam.direction = to_vec3(vm["vd"].as<std::vector<float>>(), "vd");
        if (vm.count("up")) config.cam.up = to_vec3(vm["up"].as<std::vector<float>>(), "up");
        
        if (vm.count("res")) {
            auto res = vm["res"].as<std::vector<int>>();
            if (res.size() == 2) config.resolution = glm::ivec2(res[0], res[1]);
        }

        if (vm.count("point_lights")) {
            const auto& raw_data = vm["point_lights"].as<std::vector<float>>();
            
            // Walidacja: czy liczba parametrów jest podzielna przez 7?
            if (raw_data.size() % 7 != 0) {
                throw std::runtime_error("Błędna liczba parametrów dla --point_lights! "
                                            "Każde światło musi mieć 7 wartości (x,y,z,r,g,b,s).");
            }

            // Przetwarzanie paczek po 7 wartości
            for (size_t i = 0; i < raw_data.size(); i += 7) {
                PointLightSource light;
                light.position = glm::vec3(raw_data[i],   raw_data[i+1], raw_data[i+2]);
                light.color    = FloatColor(raw_data[i+3], raw_data[i+4], raw_data[i+5], 1.0f);
                light.strength = raw_data[i+6];
                
                config.point_lights.push_back(light);
            }
        }

        if (vm.count("sphere_lights")) {
            const auto& raw_data = vm["sphere_lights"].as<std::vector<float>>();
            
            // Walidacja: czy liczba parametrów jest podzielna przez 7?
            if (raw_data.size() % 8 != 0) {
                throw std::runtime_error("Błędna liczba parametrów dla --point_lights! "
                                            "Każde światło musi mieć 8 wartości (x,y,z,r,g,b,s,r).");
            }

            // Przetwarzanie paczek po 8 wartości
            for (size_t i = 0; i < raw_data.size(); i += 8) {
                SphereLightSource light = SphereLightSource{
                    glm::vec3(raw_data[i],   raw_data[i+1], raw_data[i+2]),
                    raw_data[i+7],
                    FloatColor(raw_data[i+3], raw_data[i+4], raw_data[i+5], 1.0f),
                    raw_data[i+6]
                };
                config.sphere_lights.push_back(light);
            }
        }
    } catch(const std::exception& e) {
        log_err("Niepoprawne argumenty: ", e.what());
        exit(-1);
    }

    run_scene_form_config(config);

    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) return boot_from_params(argc, argv);

    auto scene_0 = Demo::setup_cornell_box();
    Demo::live_preview(std::move(scene_0));
    // Demo::render_picture(std::move(scene_0), "./scene_0.jpg");

    // auto scene_1 = Demo::setup_sponza();
    // Demo::render_picture(scene_1, "./scene_1.jpg");

    // auto scene_2 = Demo::setup_breakfast_room();
    // Demo::live_preview(scene_2);
    // Demo::render_picture(std::move(scene_2), "./scene_2_after_rework.jpg");

    return 0;
}