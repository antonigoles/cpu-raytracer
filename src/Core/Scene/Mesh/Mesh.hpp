#pragma once
#include <vector>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <Core/Scene/Texture/Texture.hpp>
#include <Core/Scene/Material/Material.hpp>
#include <Core/Scene/Triangle/Triangle.hpp>

class Mesh
{
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texture_coords;
    std::vector<Triangle> triangles;
    Texture texture;
    Material material;

    void dump_from_assimp_material_to_internal_material(aiMaterial* assimp_material);
};