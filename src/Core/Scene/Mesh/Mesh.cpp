#include <Core/Scene/Mesh/Mesh.hpp>
#include <Infrastructure/Logger/Logger.hpp>

void Mesh::dump_from_assimp_material_to_internal_material(aiMaterial* assimp_material)
{
    aiColor4D color_dump;
    float float_dump;
    int int_dump;
    aiShadingMode shading_mode_dump;

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
        if (material.emission.red + material.emission.green + material.emission.blue > 0.0f) {
            material.is_emissive = true;
        }
    }

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_EMISSIVE_INTENSITY, color_dump)) {
        auto em_intensity = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
        if (material.emission.red + material.emission.green + material.emission.blue == 0.0f) {
            material.emission = FloatColor(1.0f, 1.0f, 1.0f);
        }
        material.emission.red *= em_intensity.red;
        material.emission.green *= em_intensity.green;
        material.emission.blue *= em_intensity.blue;
        material.emission.alpha *= em_intensity.alpha;
        material.is_emissive = true;
    }

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_SHININESS, float_dump)) {
        material.shininess = float_dump; 
    }

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_OPACITY, float_dump)) {
        material.opacity = float_dump; 
    }

    // note: this is basically useless
    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_SHADING_MODEL, shading_mode_dump)) {
        material.assimp_shading_mode = shading_mode_dump;
    }
}