#include <Core/Scene/Mesh/Mesh.hpp>

void Mesh::dump_from_assimp_material_to_internal_material(aiMaterial* assimp_material)
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
}