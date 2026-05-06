#include <Core/Scene/SceneLoader/SceneLoader.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <filesystem>

bool SceneLoader::iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(),
                    b.begin(), b.end(),
                    [](char a, char b) {
                        return std::tolower(a) == std::tolower(b);
                    });
}

SceneLoader::SceneLoader() {};

/**
 * This method works on both Linux and Windows - and works with stupid models made on Windows
 */
std::string SceneLoader::make_path_from_source_and_texture(const std::string& obj_file_path, aiString texture_path) 
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

std::unique_ptr<Scene> SceneLoader::load_scene_from_file(const std::string& file) {
    auto new_scene = std::make_unique<Scene>();
    Assimp::Importer importer;
    
    log_info("Loading scene: ", file);

    const aiScene* ai_scene = importer.ReadFile(
        file,
        aiProcess_Triangulate | 
        aiProcess_PreTransformVertices |
        aiProcess_GenSmoothNormals // aiProcess_GenNormals for hard edges
    );

    if (!ai_scene) {
        log_err("Could not load scene: ", file, "\nReason: ", importer.GetErrorString());        
        exit(-1);
    }

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
            new_scene->meshes.push_back(internal_mesh);
        }
    }


    // build emissive triangle list
    for (auto& mesh : new_scene->meshes) {
        if (mesh.material.is_emissive) {
            new_scene->emissive_triangles.insert(mesh);
        }
    }
    // new_scene->emissive_triangles.debug_log_content();
    return new_scene;
};