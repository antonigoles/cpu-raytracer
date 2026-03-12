#include <iostream>
#include <stdint.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <ctime>
#include <thread>
#include <filesystem>

#include <LivePreview.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION 
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION 
#include <stb/stb_image_resize.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <embree4/rtcore.h>

#define ENABLE_VERBOSE true

enum class LogLevel {
    VERBOSE,
    INFO,
    WARNING,
    ERR
};

std::mutex log_mutex;
template <typename... Args>
void log(LogLevel level, Args&&... args) {
    std::lock_guard<std::mutex> guard(log_mutex);

    switch (level) {
        case LogLevel::VERBOSE:
            if (ENABLE_VERBOSE) std::cout << "\033[32m[VERBOSE]\033[0m ";
            break;
        case LogLevel::INFO:
            std::cout << "\033[32m[INFO]\033[0m ";
            break;
        case LogLevel::WARNING:
            std::cout << "\033[33m[WARN]\033[0m ";
            break;
        case LogLevel::ERR:
            std::cerr << "\033[31m[ERROR]\033[0m ";
            (std::cerr << ... << std::forward<Args>(args)) << std::endl;
            return;
    }
    
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// Wygodne wrappery, żeby pisać jeszcze mniej kodu:
template <typename... Args> void log_verbose(Args&&... args) { log(LogLevel::VERBOSE, std::forward<Args>(args)...); }
template <typename... Args> void log_info(Args&&... args) { log(LogLevel::INFO, std::forward<Args>(args)...); }
template <typename... Args> void log_warn(Args&&... args) { log(LogLevel::WARNING, std::forward<Args>(args)...); }
template <typename... Args> void log_err(Args&&... args)  { log(LogLevel::ERR, std::forward<Args>(args)...); }

class FloatColor {
public:
    float red;
    float green;
    float blue;
    float alpha;

    FloatColor operator+(const FloatColor& other) const {
        return FloatColor{
            .red   = red + other.red,
            .green = green + other.green,
            .blue  = blue + other.blue,
            .alpha = alpha + other.alpha
        };
    }

    FloatColor operator-(const FloatColor& other) const {
        return FloatColor{
            .red   = red - other.red,
            .green = green - other.green,
            .blue  = blue - other.blue,
            .alpha = alpha - other.alpha
        };
    }

    FloatColor operator*(const FloatColor& other) const {
        return FloatColor{
            .red   = red * other.red,
            .green = green * other.green,
            .blue  = blue * other.blue,
            .alpha = alpha * other.alpha
        };
    }

    FloatColor operator*(float scalar) const {
        return FloatColor{
            .red   = red * scalar,
            .green = red * scalar,
            .blue  = red * scalar,
            .alpha = red * scalar
        };
    }

    friend FloatColor operator*(float scalar, const FloatColor& color) {
        return color * scalar;
    }
};

class Color {
public:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;

    static Color from_floats(float red, float green, float blue, float alpha)
    {
        return Color{
            .red = (uint8_t)(red >= 1.0f ? 255 : red * 255.0f),
            .green = (uint8_t)(green >= 1.0f ? 255 : green * 255.0f),
            .blue = (uint8_t)(blue >= 1.0f ? 255 : blue * 255.0f),
            .alpha = (uint8_t)(alpha >= 1.0f ? 255 : alpha * 255.0f),
        };
    }

    FloatColor as_floats() const {
        return FloatColor{ (float)red / 255.0f, (float)green / 255.0f, (float)blue / 255.0f, (float)alpha / 255.0f };
    }

    Color pow(float n)
    {
        auto v = this->as_floats();
        return Color::from_floats(
            glm::pow(v.red, n),
            glm::pow(v.green, n),
            glm::pow(v.blue, n),
            glm::pow(v.alpha, n)
        );
    }

    Color operator+(const Color& other) const {
        return Color{
            .red   = static_cast<uint8_t>(std::clamp(red + other.red, 0, 255)),
            .green = static_cast<uint8_t>(std::clamp(green + other.green, 0, 255)),
            .blue  = static_cast<uint8_t>(std::clamp(blue + other.blue, 0, 255)),
            .alpha = static_cast<uint8_t>(std::clamp(alpha + other.alpha, 0, 255))
        };
    }

    Color operator-(const Color& other) const {
        return Color{
            .red   = static_cast<uint8_t>(std::clamp(red - other.red, 0, 255)),
            .green = static_cast<uint8_t>(std::clamp(green - other.green, 0, 255)),
            .blue  = static_cast<uint8_t>(std::clamp(blue - other.blue, 0, 255)),
            .alpha = static_cast<uint8_t>(std::clamp(alpha - other.alpha, 0, 255))
        };
    }

    Color operator*(const Color& other) const {
        auto mult = this->as_floats() * other.as_floats();
        return Color::from_floats(mult.red, mult.green, mult.blue, mult.alpha);
    }

    Color operator*(float scalar) const {
        return Color{
            .red   = static_cast<uint8_t>(std::clamp(red * scalar, 0.0f, 255.0f)),
            .green = static_cast<uint8_t>(std::clamp(green * scalar, 0.0f, 255.0f)),
            .blue  = static_cast<uint8_t>(std::clamp(blue * scalar, 0.0f, 255.0f)),
            .alpha = static_cast<uint8_t>(std::clamp(alpha * scalar, 0.0f, 255.0f))
        };
    }

    friend Color operator*(float scalar, const Color& color) {
        return color * scalar;
    }
};

class Fragment : public Color {};

template<typename T>
class Buffer2D {
private:
    T* buffer_data;
    uint32_t width;
    uint32_t height;

public:
    Buffer2D(uint32_t width, uint32_t height) : width(width), height(height) {
        buffer_data = new T[width * height];
    };

    ~Buffer2D() {
        delete[] buffer_data;
    }

    Buffer2D(Buffer2D&& other) noexcept : width(other.width), height(other.height), buffer_data(other.buffer_data) {
        // "Zerujemy" obiekt źródłowy, żeby jego destruktor nie usunął naszej pamięci!
        other.buffer_data = nullptr; 
        other.width = 0;
        other.height = 0;
    }

    Buffer2D(const Buffer2D&) = delete;
    Buffer2D& operator=(const Buffer2D&) = delete;

    // TODO: Implement rescale

    uint32_t get_width() {
        return width;
    }

    uint32_t get_height() {
        return height;
    }

    T* at(uint32_t x, uint32_t y) {
        return &this->buffer_data[this->width * y + x];
    }

    void write(uint32_t x, uint32_t y, T data) {
        this->buffer_data[this->width * y + x] = data;
    }

    uint8_t* bytes() {
        return (uint8_t*)buffer_data;
    }
};

class TextureMimap
{
public:
    int width, height, channels;
    std::vector<uint8_t> data;

    void load(const std::string& filepath, uint32_t level)
    {
        uint8_t* img = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
        if (!img) {
            log_err("Could not load texture: ", filepath);
            exit(-1);
        }

        uint32_t divisor = glm::pow(2, level);

        uint32_t new_width = width / divisor;
        uint32_t new_height = height / divisor;

        uint8_t* resized_pixels = (uint8_t*)malloc(new_width * new_height * 4);
        stbir_resize_uint8(img, width, height, 0, resized_pixels, new_width, new_height, 0, 4);
        stbi_image_free(img);
        data.assign(resized_pixels, (resized_pixels + new_height * new_width * 4));

        width = new_width;
        height = new_height;
        free(resized_pixels);
    }

    Color sample(float u, float v) const {
        if (data.empty()) return Color::from_floats(1.0f, 0.0f, 1.0f, 1.0f);

        u = u - std::floor(u);
        v = v - std::floor(v);

        int x = (int)(u * width) % width;
        
        int y = (int)((1.0f - v) * height) % height; 

        int index = (y * width + x) * 4;

        return Color{
            .red   = data[index + 0],
            .green = data[index + 1],
            .blue  = data[index + 2],
            .alpha = data[index + 3]
        };
    }
};

class Texture {
public:
    bool is_empty;
    std::vector<TextureMimap> mipmaps;

    Texture() {
        // empty texture
        mipmaps = {};
        is_empty = true;
    }

    Texture(const std::string& filepath) {
        for (int i = 0; i <= 4; i++) {
            TextureMimap mipmap;
            mipmap.load(filepath, i);
            this->mipmaps.push_back(std::move(mipmap));
        }
        is_empty = false;
    }

    Color sample(float u, float v, float distance) const {
        if (mipmaps.empty()) return Color(0.0f, 0.0f, 0.0f, 0.0f);
        uint32_t mipmap_idx = std::clamp((int)(distance / 6.0), 0, (int)mipmaps.size() - 1);
        return mipmaps[mipmap_idx].sample(u, v);
    }
};


class ImageWriter
{
public:
    ImageWriter() {};

    void write_jpg_from_frame_buffer(Buffer2D<Fragment> *buffer, const std::string& path) {
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

    void write_vide_from_buffer_vector(std::vector<Buffer2D<Fragment>>& frames, const std::string& filepath, int fps = 60) 
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
};

class Triangle
{
public:
    uint32_t indices[3];
};

class Material 
{
public:
    FloatColor diffuse = Color(0,0,0,0).as_floats();
    FloatColor specular = Color(0,0,0,0).as_floats();
    FloatColor ambient = Color(0,0,0,0).as_floats();
    FloatColor emission = Color(0,0,0,0).as_floats();
    // FloatColor emission_strength = Color(0,0,0,0).as_floats();
    float shininess = 1;
    float opacity = 0; 
    int illumination = 0;
};

class Mesh
{
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texture_coords;
    std::vector<Triangle> triangles;
    Texture texture;
    Material material;

    void dump_from_assimp_material_to_internal_material(aiMaterial* assimp_material)
    {
        aiColor4D color_dump;
        float float_dump;
        int int_dump;

        material.illumination = 2;

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_DIFFUSE, color_dump)) {
            material.diffuse = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
        }

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_SPECULAR, color_dump)) {
            material.specular = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
        }

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_AMBIENT, color_dump)) {
            material.ambient = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
        }

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_EMISSIVE, color_dump)) {
            material.emission = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
        }

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_SHININESS, float_dump)) {
            material.shininess = float_dump; 
            if (float_dump != 1.0) material.illumination = 4;
        }

        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_OPACITY, float_dump)) {
            material.shininess = float_dump; 
        }

        // if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_SHADING_MODEL, int_dump)) {
        //     material.illumination = int_dump; 
        // }
    }
};

class PointLightSource
{
public:
    glm::vec3 position;
    FloatColor color;
    float strength;

    FloatColor get_effective_color(float r, float cos_theta) {
        return color * ((strength * cos_theta) / (4.0f * 3.14159f * r * r));
    }
};

class TriangleLightSource
{
public:
    glm::vec3 points[3];
    glm::vec3 normal;
    FloatColor emissive_color = Color(255, 255, 255, 255).as_floats();
    float strength = 10.0f;
    float pdf;

    TriangleLightSource(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        points[0] = a;
        points[1] = b;
        points[2] = c;
        auto crss = glm::cross(points[1] - points[0], points[2] - points[0]);
        normal = glm::normalize(crss);
        pdf = glm::length(crss) / 2.0f;
    }

    glm::vec3 sample_random_point() {
        float r1 = static_cast<float>(rand()) / RAND_MAX;
        float r2 = static_cast<float>(rand()) / RAND_MAX;
        float sqrt_r1 = std::sqrt(r1);
        float u = 1.0f - sqrt_r1;
        float v = r2 * sqrt_r1;
        return points[0] * (1.0f - sqrt_r1) + points[1] * (sqrt_r1 * (1.0f - r2)) + points[2] * (r2 * sqrt_r1);
    }

    // Solid Angle PDF
    FloatColor get_effective_color(float r, float cos_theta_i, float cos_theta_l) {
        return emissive_color * ((strength * cos_theta_i * cos_theta_l) / (pdf * r * r));
    }
};

class SphereLightSource
{
public:
    glm::vec3 center;
    float radius;
    FloatColor emissive_color = Color(255, 255, 255, 255).as_floats();
    float strength = 10.0f;

    SphereLightSource(glm::vec3 center, float radius, FloatColor emissive_color, float strength) 
    : center(center), radius(radius), emissive_color(emissive_color), strength(strength) {};

    glm::vec3 get_closest_point(glm::vec3 from) {
        return center + glm::normalize(center - from) * radius;
    }

    FloatColor get_effective_color(float r, float cos_theta) {
        return emissive_color * ((strength * cos_theta) / (r * r));
    }
};


class Camera 
{
public:
    Camera() {};

    float fov;

    glm::vec3 position;
    glm::quat rotation;

    glm::vec3 forward()
    {
        return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 right()
    {
        return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 up()
    {
        return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

void handle_embree_error(void* userPtr, enum RTCError error, const char* str) {
    log_err("[Embree Error] ", error, ": ", str);
};

class Scene 
{
public:
    std::vector<Mesh> meshes;
    std::shared_ptr<Camera> camera;
    std::vector<PointLightSource> point_light_sources;
    std::vector<TriangleLightSource> triangle_light_sources;
    std::vector<SphereLightSource> sphere_light_sources;

    RTCDevice embree_device = nullptr;
    RTCScene embree_scene = nullptr;

    ~Scene() 
    {
        if (embree_scene) rtcReleaseScene(embree_scene);
        if (embree_device) rtcReleaseDevice(embree_device);
    }

    void build_embree()
    {
        log_info("Building embree");
        embree_device = rtcNewDevice(nullptr);
        if (!embree_device) {
            log_err("Critical: Could not create Embree device!");
            exit(-1);
        }
        rtcSetDeviceErrorFunction(embree_device, handle_embree_error, nullptr);

        embree_scene = rtcNewScene(embree_device);

        for (size_t mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) 
        {
            const Mesh& mesh = meshes[mesh_idx];
            
            if (mesh.vertices.empty() || mesh.triangles.empty()) continue;

            RTCGeometry geom = rtcNewGeometry(embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

            glm::vec3* embree_vertices = (glm::vec3*)rtcSetNewGeometryBuffer(
                geom, 
                RTC_BUFFER_TYPE_VERTEX, 
                0, 
                RTC_FORMAT_FLOAT3, 
                sizeof(glm::vec3), 
                mesh.vertices.size()
            );
            

            std::memcpy(embree_vertices, mesh.vertices.data(), mesh.vertices.size() * sizeof(glm::vec3));

            Triangle* embree_indices = (Triangle*)rtcSetNewGeometryBuffer(
                geom, 
                RTC_BUFFER_TYPE_INDEX, 
                0, 
                RTC_FORMAT_UINT3, 
                sizeof(Triangle), 
                mesh.triangles.size()
            );

            std::memcpy(embree_indices, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));

            rtcCommitGeometry(geom);
            
            unsigned int geomID = rtcAttachGeometry(embree_scene, geom);
            
            rtcReleaseGeometry(geom);
        }

        rtcCommitScene(embree_scene);
        log_info("Embree built successfully");
    }
};

class SceneLoader
{
private:
    bool iequals(const std::string& a, const std::string& b) {
        return std::equal(a.begin(), a.end(),
                        b.begin(), b.end(),
                        [](char a, char b) {
                            return std::tolower(a) == std::tolower(b);
                        });
    }
public:
    SceneLoader() {};

    /**
     * This method works on both Linux and Windows - and works with stupid models made on Windows
     */
    std::string make_path_from_source_and_texture(const std::string& obj_file_path, aiString texture_path) 
    {
        std::filesystem::path obj_path(obj_file_path);
        std::filesystem::path base_dir = obj_path.parent_path();
        
        std::string tex_str = texture_path.C_Str();
        std::replace(tex_str.begin(), tex_str.end(), '\\', '/');
        
        std::filesystem::path tex_path(tex_str);
        std::filesystem::path final_path = base_dir / tex_path;

        if (std::filesystem::exists(final_path)) {
            return final_path.string();
        }

        std::filesystem::path expected_dir = final_path.parent_path();
        std::string expected_filename = final_path.filename().string();

        if (std::filesystem::exists(expected_dir) && std::filesystem::is_directory(expected_dir)) 
        {
            for (const auto& entry : std::filesystem::directory_iterator(expected_dir)) 
            {
                if (entry.is_regular_file()) 
                {
                    std::string actual_filename = entry.path().filename().string();
                    
                    if (iequals(expected_filename, actual_filename)) {
                        return entry.path().string();
                    }
                }
            }
        }

        return final_path.string();
    }

    Scene load_scene_from_file(const std::string& file) {
        Scene new_scene;
        Assimp::Importer importer;
        
        log_info("Loading scene: ", file);

        const aiScene* ai_scene = importer.ReadFile(
            file,
            aiProcess_Triangulate | 
            aiProcess_PreTransformVertices |
            aiProcess_GenSmoothNormals // aiProcess_GenNormals for hard edges
        );

        if (ai_scene->HasMeshes()) {
            for (uint32_t i = 0; i < ai_scene->mNumMeshes; i++) {
                aiMesh* mesh = ai_scene->mMeshes[i];
                unsigned int materialIndex = mesh->mMaterialIndex;

                Mesh internal_mesh;
                auto assimp_material = ai_scene->mMaterials[materialIndex];
                aiVector3D* vertices = mesh->mVertices;
                aiVector3D* normals = mesh->mNormals;
                aiVector3D* texture_coords = mesh->mTextureCoords[0];

                for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                    internal_mesh.vertices.push_back(glm::vec3(vertices[j].x, vertices[j].y, vertices[j].z));
                    internal_mesh.normals.push_back(glm::vec3(normals[j].x, normals[j].y, normals[j].z));
                    if (mesh->HasTextureCoords(0)) {
                        internal_mesh.texture_coords.push_back(glm::vec2(texture_coords[j].x, texture_coords[j].y));
                    } else {
                        internal_mesh.texture_coords.push_back(glm::vec2(0.0f, 0.0f));
                    }
                }

                for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
                    aiFace face = mesh->mFaces[j];
                    Triangle trig;
                    for (int i = 0; i<3; i++) {
                        trig.indices[i] = face.mIndices[i];
                    }
                    internal_mesh.triangles.push_back(trig);
                }

                aiString texture_path;
                if (assimp_material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS) {
                    internal_mesh.texture = Texture(make_path_from_source_and_texture(file, texture_path));
                }

                internal_mesh.dump_from_assimp_material_to_internal_material(assimp_material);

                new_scene.meshes.push_back(internal_mesh);
            }
        }

        return new_scene;
    };
};

class Ray 
{
public:
    glm::vec3 base;
    glm::vec3 direction;

    RTCRay get_embree_ray(float tnear = 0.0001f, float tfar = std::numeric_limits<float>::infinity()) const
    {
        RTCRay query;

        query.org_x = base.x;
        query.org_y = base.y;
        query.org_z = base.z;

        query.dir_x = direction.x;
        query.dir_y = direction.y;
        query.dir_z = direction.z;

        query.tnear = tnear;
        query.tfar = tfar;

        query.mask = -1;
        query.flags = 0;

        return query;
    }

    RTCRayHit get_embree_ray_hit(float tnear = 0.0001f, float tfar = std::numeric_limits<float>::infinity()) const
    {
        RTCRayHit query;

        query.ray.org_x = base.x;
        query.ray.org_y = base.y;
        query.ray.org_z = base.z;

        query.ray.dir_x = direction.x;
        query.ray.dir_y = direction.y;
        query.ray.dir_z = direction.z;

        query.ray.tnear = tnear;
        query.ray.tfar = tfar;

        query.ray.mask = -1;
        query.ray.flags = 0;

        query.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        query.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

        return query;
    }
};

class BasicRayTracer
{
public:
    BasicRayTracer() {};

    std::vector<FloatColor> gather_light_color(const Scene& scene, const glm::vec3 point, const glm::vec3 normal, const glm::vec3 geometric_normal, float shinines = 1.0f) {
        FloatColor diffuse = FloatColor(0,0,0,0);
        FloatColor specular = FloatColor(0,0,0,0);

        for (auto point_light_source : scene.point_light_sources) {
            auto norm_direction_to_light = glm::normalize(point_light_source.position - point);
            glm::vec3 safe_geometric_normal = geometric_normal;

            glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

            Ray light_ray = {
                .base = safe_shadow_origin,
                .direction = norm_direction_to_light
            };
  
            auto distance_to_light = glm::distance(point, point_light_source.position);
            auto embree_light_ray = light_ray.get_embree_ray(0.0001f, distance_to_light - 0.001f);
            
            RTCOccludedArguments args;
            rtcInitOccludedArguments(&args);
            rtcOccluded1(scene.embree_scene, &embree_light_ray, &args);

            if (embree_light_ray.tfar <= 0.0f) continue;

            diffuse = diffuse + point_light_source.get_effective_color(
                distance_to_light,
                glm::max(0.0f, glm::dot(normal, norm_direction_to_light))
            );
        }


        for (auto triangle_light_source : scene.triangle_light_sources) {
            // randomly sampled point on triangle
            auto light_point = triangle_light_source.sample_random_point();

            auto norm_direction_to_light = glm::normalize(light_point - point);
            glm::vec3 safe_geometric_normal = geometric_normal;

            glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

            Ray light_ray = {
                .base = safe_shadow_origin,
                .direction = norm_direction_to_light
            };

            
            auto distance_to_light = glm::distance(point, light_point);
            auto embree_light_ray = light_ray.get_embree_ray(0.0001f, distance_to_light - 0.001f);
            
            RTCOccludedArguments args;
            rtcInitOccludedArguments(&args);
            rtcOccluded1(scene.embree_scene, &embree_light_ray, &args);

            if (embree_light_ray.tfar <= 0.0f) continue;

            diffuse = diffuse + triangle_light_source.get_effective_color(
                distance_to_light,
                std::max(0.0f, glm::dot(normal, norm_direction_to_light)),
                std::max(0.0f, glm::dot(triangle_light_source.normal, -norm_direction_to_light))
            );

            glm::vec3 Lj = light_point - point;
            glm::vec3 V = scene.camera->position - point;
            glm::vec3 Hj = glm::normalize(Lj + V);

            specular = specular + triangle_light_source.get_effective_color(
                distance_to_light,
                glm::pow(std::max(0.0f, glm::dot(normal, Hj)), shinines),
                std::max(0.0f, glm::dot(triangle_light_source.normal, -norm_direction_to_light))
            );
        }

        for (auto sphere_light_source : scene.sphere_light_sources) {
            // randomly sampled point on triangle
            auto light_point = sphere_light_source.get_closest_point(point);

            auto norm_direction_to_light = glm::normalize(light_point - point);
            glm::vec3 safe_geometric_normal = geometric_normal;

            glm::vec3 safe_shadow_origin = point + (safe_geometric_normal * 0.001f);

            Ray light_ray = {
                .base = safe_shadow_origin,
                .direction = norm_direction_to_light
            };

            
            auto distance_to_light = glm::distance(point, light_point);
            auto embree_light_ray = light_ray.get_embree_ray(0.0001f, distance_to_light - 0.001f);
            
            RTCOccludedArguments args;
            rtcInitOccludedArguments(&args);
            rtcOccluded1(scene.embree_scene, &embree_light_ray, &args);

            if (embree_light_ray.tfar <= 0.0f) continue;

            diffuse = diffuse + sphere_light_source.get_effective_color(
                distance_to_light,
                std::max(0.0f, glm::dot(normal, norm_direction_to_light))
            );

            glm::vec3 Lj = light_point - point;
            glm::vec3 V = scene.camera->position - point;
            glm::vec3 Hj = glm::normalize(Lj + V);

            specular = specular + sphere_light_source.get_effective_color(
                distance_to_light,
                glm::pow(std::max(0.0f, glm::dot(normal, Hj)), shinines)
            );
        }

        return {diffuse, specular};
    }

    FloatColor cast_ray(const Ray& ray, const Scene& scene, uint32_t depth_left = 8) {
        auto color = FloatColor{0, 0, 0, 255};
        if (depth_left == 0) {
            return color;
        }

        RTCRayHit embree_ray = ray.get_embree_ray_hit();

        RTCIntersectArguments args;
        rtcInitIntersectArguments(&args);
        rtcIntersect1(scene.embree_scene, &embree_ray, &args);

        auto ambient_light = Color(20, 30, 50, 255).as_floats();

        if (embree_ray.hit.geomID != RTC_INVALID_GEOMETRY_ID)
        {
            uint32_t mesh_idx = embree_ray.hit.geomID;
            uint32_t triangle_idx = embree_ray.hit.primID;
            const Triangle& triangle = scene.meshes[mesh_idx].triangles[triangle_idx];
            const Mesh& mesh = scene.meshes[mesh_idx];

            float u = embree_ray.hit.u;
            float v = embree_ray.hit.v;
            float w = 1.0f - u - v;

            glm::vec3 interpolated_point = ray.base + (ray.direction * embree_ray.ray.tfar);

            glm::vec3 n0 = scene.meshes[mesh_idx].normals[triangle.indices[0]];
            glm::vec3 n1 = scene.meshes[mesh_idx].normals[triangle.indices[1]];
            glm::vec3 n2 = scene.meshes[mesh_idx].normals[triangle.indices[2]];

            glm::vec3 p0 = scene.meshes[mesh_idx].vertices[triangle.indices[0]];
            glm::vec3 p1 = scene.meshes[mesh_idx].vertices[triangle.indices[1]];
            glm::vec3 p2 = scene.meshes[mesh_idx].vertices[triangle.indices[2]];

            glm::vec3 geometric_normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

            glm::vec3 interpolated_normal = (w * n0) + (u * n1) + (v * n2);
            // interpolated_normal = glm::normalize( interpolated_normal);

            if (glm::dot(geometric_normal, interpolated_normal) < 0) {
                geometric_normal = -geometric_normal;
            }

            Ray next_ray = Ray{
                .base = interpolated_point,
                .direction = glm::reflect(ray.direction, interpolated_normal) 
            };
            
            auto diffuse_reflectance = mesh.material.diffuse;

            if (!mesh.texture.is_empty)
            {
                glm::vec2 txt_uv_0 = scene.meshes[mesh_idx].texture_coords[triangle.indices[0]];
                glm::vec2 txt_uv_1 = scene.meshes[mesh_idx].texture_coords[triangle.indices[1]];
                glm::vec2 txt_uv_2 = scene.meshes[mesh_idx].texture_coords[triangle.indices[2]];

                glm::vec2 texture_uv = (w * txt_uv_0) + (u * txt_uv_1) + (v * txt_uv_2);

                diffuse_reflectance = mesh.texture.sample(texture_uv.x, texture_uv.y, embree_ray.ray.tfar).as_floats();
            }

            if (mesh.material.illumination == 0) {
                return diffuse_reflectance;
            }

            std::vector<FloatColor> light_color = gather_light_color(scene, interpolated_point, interpolated_normal, geometric_normal, mesh.material.shininess);


            // if (mesh.material.illumination == 1) {
            //     return mesh.material.ambient * ambient_light + diffuse_reflectance * light_color[0];
            // }

            // Color light_color_shinines = gather_light_color(scene, interpolated_point, interpolated_normal, geometric_normal, mesh.material.shininess);

            // if (mesh.material.illumination == 2) {
            //     return mesh.material.ambient * ambient_light + diffuse_reflectance * light_color[0] + mesh.material.specular;
            // }

            FloatColor recast_result = cast_ray(next_ray, scene, depth_left - 1);

            ambient_light = diffuse_reflectance * 0.1f;

            // if (mesh.material.illumination <= 4) {
                
                // return mesh.material.ambient * ambient_light 
                // + diffuse_reflectance * light_color[0]
                // + mesh.material.specular * (light_color[1] + recast_result);
            // }

            return mesh.material.ambient * ambient_light 
                + diffuse_reflectance * light_color[0]
                + mesh.material.specular * (light_color[1] + recast_result);
        }

        return color;
    };

    Buffer2D<Fragment> ray_trace_scene(const Scene& scene, uint32_t width, uint32_t height, int sample_per_pixel = 1, float jitter_scale = 0.25f)
    {
        if (scene.embree_scene == nullptr) {
            log_err("Embree scene not initialized");
            exit(-1);
        }
        
        Buffer2D<Fragment> buffer(width, height);

        float hfov_tan = glm::tan(scene.camera->fov / 2.0f);
        float aspect_ratio = ((float)width) / ((float)height);
        
        glm::vec3 cam_pos = scene.camera->position;
        glm::vec3 cam_right = scene.camera->right();
        glm::vec3 cam_up = scene.camera->up();
        glm::vec3 cam_forward = scene.camera->forward();

        std::atomic<uint32_t> current_y{0};

        glm::vec2 sample_center = glm::vec2((float)sample_per_pixel / 2.0f, (float)sample_per_pixel / 2.0f);

        auto worker_thread = [&]() 
        {
            uint32_t y;
            
            while ((y = current_y.fetch_add(1)) < height) 
            {
                float p_y = hfov_tan * (1.0f - ((float)y + 0.5f) / ((float)height) * 2.0f);

                for (uint32_t x = 0; x < width; x++) 
                {
                    // TODO: Maybe make weighted sampling
                    FloatColor sum_color = Color(0, 0, 0, 0).as_floats();
                    for (int sy = 0; sy < sample_per_pixel; sy++) {
                        for (int sx = 0; sx < sample_per_pixel; sx++) {
                            glm::vec2 sample_point = glm::vec2((float)sy, (float)sx);
                            glm::vec2 sample_jitter = (sample_point - sample_center) * jitter_scale;

                            float p_x = hfov_tan * aspect_ratio * (((float)x + 0.5f) / ((float)width) * 2.0f - 1.0f);

                            float p_y_prim = p_y + sample_jitter.y;
                            float p_x_prim = p_x + sample_jitter.x;

                            Ray ray {
                                .base = cam_pos,
                                .direction = glm::normalize(p_x_prim * cam_right + p_y_prim * cam_up + cam_forward)
                            };

                            sum_color = sum_color + this->cast_ray(ray, scene);
                        }
                    }
                    
                    buffer.write(x, y, Fragment(
                        Color::from_floats(sum_color.red, sum_color.green, sum_color.blue, sum_color.alpha) 
                        * (1.0f / (sample_per_pixel * sample_per_pixel))
                    ));
                }
            }
        };

        uint32_t num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (uint32_t i = 0; i < num_threads; i++) {
            threads.emplace_back(worker_thread);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        return buffer;
    }
};

template<typename T>
T lerp(T position, T target, float delta_time)
{
    return position + (target - position) * delta_time;
}

glm::quat EulerToQuatRadians(float pitch, float yaw, float roll) {
    // Kolejność to zazwyczaj X (pitch), Y (yaw), Z (roll)
    glm::vec3 eulerAngles(pitch, yaw, roll);
    
    // Zwracamy kwaternion utworzony bezpośrednio z wektora kątów Eulera
    return glm::quat(eulerAngles);
}

void animation_test(Scene& scene) {
    auto image_writer = ImageWriter();
    auto ray_tracer = BasicRayTracer();

    std::vector<Buffer2D<Fragment>> video_buffer;

    const uint32_t frame_count = 240;

    for (uint32_t i = 1; i <= frame_count; i++) {
        scene.camera->position.x += 0.05f;
        scene.camera->position.y += 0.15f;
        scene.camera->rotation = EulerToQuatRadians(glm::radians(-(float)i / 3.0f), glm::radians(90.0f), 0.0f);

        Buffer2D<Fragment> ray_traced_buffer = ray_tracer.ray_trace_scene(scene, 512, 512);
        video_buffer.push_back(std::move(ray_traced_buffer));
        log_info("Frame ", i, " out of ", frame_count);
    }

    image_writer.write_vide_from_buffer_vector(video_buffer, "./output.mp4", 60);
}

void render_picture(const Scene& scene, std::string as = "./output.jpg", int sample_per_pixel = 1, float jitter_scale = (0.25F)) {
    auto image_writer = ImageWriter();
    auto ray_tracer = BasicRayTracer();

    auto ray_traced_buffer = ray_tracer.ray_trace_scene(scene, 4120, 3120, sample_per_pixel, jitter_scale);
    image_writer.write_jpg_from_frame_buffer(&ray_traced_buffer, as);
}

void live_preview(Scene& scene)
{
    auto ray_tracer = BasicRayTracer();
    auto live_preview = LivePreview(1280, 720);

    live_preview.init_window();
    float delta_time = 0.01f;

    float camera_angle_sideways = glm::radians(90.0f);
    float camera_angle_upwards = glm::radians(-20.0f);

    while (!live_preview.window_should_close()) {
        auto timestamp = glfwGetTime();
        auto ray_traced_buffer = ray_tracer.ray_trace_scene(scene, 1280, 720);
        live_preview.load_frame(ray_traced_buffer.bytes());
        delta_time = glfwGetTime() - timestamp;
        // log_info(1.0f / delta_time, " FPS");

        // Controls
        glm::vec3 move = glm::vec3(0.0f,0.0f,0.0f);

        float multiplier = live_preview.is_held(GLFW_KEY_LEFT_SHIFT) ? 2.0f : 1.0f;

        if (live_preview.is_held(GLFW_KEY_W)) move += scene.camera->forward();
        if (live_preview.is_held(GLFW_KEY_S)) move -= scene.camera->forward();
        if (live_preview.is_held(GLFW_KEY_A)) move -= scene.camera->right();
        if (live_preview.is_held(GLFW_KEY_D)) move += scene.camera->right();

        if (live_preview.is_held(GLFW_KEY_UP)) camera_angle_upwards += multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_DOWN)) camera_angle_upwards -= multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_LEFT)) camera_angle_sideways += multiplier * delta_time;
        if (live_preview.is_held(GLFW_KEY_RIGHT)) camera_angle_sideways -= multiplier * delta_time;

        if (live_preview.is_held(GLFW_KEY_P)) {
            scene.sphere_light_sources[0].radius += delta_time;
            scene.sphere_light_sources[1].radius += delta_time;
        }

        if (live_preview.is_held(GLFW_KEY_O)) {
            scene.sphere_light_sources[0].radius -= delta_time;
            scene.sphere_light_sources[1].radius -= delta_time;
        }

        scene.camera->rotation = EulerToQuatRadians(camera_angle_upwards, camera_angle_sideways, 0.0f);
        scene.camera->position += glm::length(move) == 0 ? move : glm::normalize(move) * delta_time * multiplier;

        log_info(
            "pos: ",
            scene.camera->position.x, " ", 
            scene.camera->position.y, " ", 
            scene.camera->position.z, 
            "rot: ",
            camera_angle_sideways,
            camera_angle_upwards
        );
    }
    live_preview.terminate();
}

inline Scene setup_sponza()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(10.0f, 8.8f, 0.0f);
    main_camera->rotation = EulerToQuatRadians(glm::radians(0.0f), glm::radians(90.0f), 0.0f);
    main_camera->fov = glm::radians(70.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/Sponza/sponza.obj");

    scene.point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(-5.0f, 10.0f, 0.0f),
        .color = Color(34, 56, 255, 255).as_floats(),
        .strength = 500.0f
    });

    scene.point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(0.0f, 10.0f, 0.0f),
        .color = Color(21, 188, 42, 255).as_floats(),
        .strength = 500.0f
    });

    scene.point_light_sources.push_back(PointLightSource{
        .position = glm::vec3(5.0f, 10.0f, 0.0f),
        .color = Color(156, 23, 24, 255).as_floats(),
        .strength = 500.0f
    });

    scene.camera = main_camera;

    // auto triangle_light_0 = TriangleLightSource(
    //     glm::vec3(1.0f, 15.58f, -1.0f),
    //     glm::vec3(1.0f, 15.58f, 1.0f),
    //     glm::vec3(-1.0f, 15.58f, 1.0f)
    // );

    // auto triangle_light_1 = TriangleLightSource(
    //     glm::vec3(1.0f, 15.58f, -1.0f),
    //     glm::vec3(-1.0f, 15.58f, 1.0f),
    //     glm::vec3(-1.0f, 15.58f, -1.0f)
    // );

    // triangle_light_0.strength = 100.0f;
    // triangle_light_0.emissive_color = Color(188, 188, 255, 255);
    // triangle_light_1.strength = 100.0f;
    // triangle_light_1.emissive_color = Color(188, 188, 255, 255);

    // scene.triangle_light_sources.push_back(triangle_light_0);
    // scene.triangle_light_sources.push_back(triangle_light_1);

    return scene;
}

inline Scene setup_breakfast_room()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(-3.0f, 2.0f, 3.0f);
    main_camera->rotation = EulerToQuatRadians(glm::radians(0.0f), glm::radians(-20.0f), 0.0f);
    main_camera->fov = glm::radians(70.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/breakfast_room/breakfast_room.obj");

    scene.sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(-2.0f, 4.0f, -2.0f),
        0.1f,
        Color(233, 166, 166, 255).as_floats(),
        5.0f
    ));

    scene.sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(1.0f, 4.0f, -2.0f),
        0.1f,
        Color(233, 166, 166, 255).as_floats(),
        5.0f
    ));

    scene.camera = main_camera;

    return scene;
}

inline Scene setup_cornell_box()
{
    auto main_camera = std::make_shared<Camera>();
    main_camera->position = glm::vec3(0.0f, 0.8f, 2.0f);
    main_camera->rotation = EulerToQuatRadians(glm::radians(0.0f), glm::radians(0.0f), 0.0f);
    main_camera->fov = glm::radians(60.0f);
    auto scene_loader = SceneLoader();
    auto scene = scene_loader.load_scene_from_file("./assets/CornellBox/CornellBox-Sphere.obj");
    scene.camera = main_camera;

    // Ceiling
    // v  1.0000 1.5900 -1.0400
    // v  1.0000 1.5900 0.9900
    // v  -1.0200 1.5900 0.9900
    // v  -1.0200 1.5900 -1.0400

    // auto triangle_light_0 = TriangleLightSource(
    //     glm::vec3(1.0f, 1.58f, -1.0f),
    //     glm::vec3(1.0f, 1.58f, 1.0f),
    //     glm::vec3(-1.0f, 1.58f, 1.0f)
    // );

    // auto triangle_light_1 = TriangleLightSource(
    //     glm::vec3(1.0f, 1.58f, -1.0f),
    //     glm::vec3(-1.0f, 1.58f, 1.0f),
    //     glm::vec3(-1.0f, 1.58f, -1.0f)
    // );

    // triangle_light_0.strength = 1.0f;
    // triangle_light_1.strength = 1.0f;

    // scene.triangle_light_sources.push_back(triangle_light_0);
    // scene.triangle_light_sources.push_back(triangle_light_1);

    // scene.point_light_sources.push_back(PointLightSource{
    //     .position = glm::vec3(0.0f, 1.5f, 0.0f),
    //     .color = Color(255, 255, 255, 255).as_floats(),
    //     .strength = 10.0f
    // });

    scene.sphere_light_sources.push_back(SphereLightSource(
        glm::vec3(0.0f, 1.5f, 0.0f),
        0.1f,
        Color(255, 255, 255, 255).as_floats(),
        1.0f
    ));

    return scene;
}

int main() {
    // auto scene_0 = setup_cornell_box();
    // scene_0.build_embree();
    // live_preview(scene_0);
    // render_picture(scene_0, "./scene_0.jpg", 1, 0.0005f);

    // auto scene_1 = setup_sponza();
    // scene_1.build_embree();
    // render_picture(scene_1, "./scene_1.jpg");

    auto scene_2 = setup_breakfast_room();
    scene_2.build_embree();
    // live_preview(scene_2);
    render_picture(scene_2, "./scene_2_on_floats.jpg");

    // live_preview(scene);
    return 0;
}