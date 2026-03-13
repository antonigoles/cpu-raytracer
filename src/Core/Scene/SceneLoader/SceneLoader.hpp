#pragma once
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Core/Scene/Scene.hpp>

class SceneLoader
{
private:
    bool iequals(const std::string& a, const std::string& b);
public:
    SceneLoader();

    /**
     * This method works on both Linux and Windows - and works with stupid models made on Windows
     */
    std::string make_path_from_source_and_texture(const std::string& obj_file_path, aiString texture_path);

    Scene load_scene_from_file(const std::string& file);
};