#include "Core/FloatColor/FloatColor.hpp"
#include <Core/Scene/Mesh/Mesh.hpp>
#include <Infrastructure/Logger/Logger.hpp>
#include <assimp/color4.h>
#include <assimp/material.h>

static void read_mtl_material(Material& material, aiMaterial* assimp_material)
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

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_REFRACTI, float_dump)) {
        material.refraction = float_dump;
    }   

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_TRANSPARENT, color_dump)) {
        material.transmission = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b, (float)color_dump.a);
    }
}

void read_pbr_material(PBRMaterial& material, aiMaterial* assimp_material)
{
    aiColor4D color_dump;
    float float_dump;

    if (assimp_material->Get(AI_MATKEY_BASE_COLOR, color_dump) != AI_SUCCESS) {
        assimp_material->Get(AI_MATKEY_COLOR_DIFFUSE, color_dump);
    }

    // Keep in mind: this might possibly be sRGB - TODO: Find a good way of detecting that
    material.base_color = FloatColor(color_dump.r, color_dump.g, color_dump.b);

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_EMISSIVE, color_dump)) {
        material.emission = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b);
    }

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_EMISSIVE_INTENSITY, float_dump)) {
        auto em_intensity = FloatColor((float)color_dump.r, (float)color_dump.g, (float)color_dump.b);
        if (material.emission.red + material.emission.green + material.emission.blue == 0.0f) {
            material.emission = FloatColor(1.0f, 1.0f, 1.0f);
        }
        material.emission = material.emission * float_dump;
    }

    float roughness = 0.5f;
    if (assimp_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != AI_SUCCESS) {
        float shininess = 0.0f;
        if (assimp_material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            roughness = 1.0f - glm::clamp(shininess / 1000.0f, 0.0f, 1.0f);
        }
    }
    material.roughness = roughness;

    float metalic = 0.0f;
    if (AI_SUCCESS != assimp_material->Get(AI_MATKEY_METALLIC_FACTOR, metalic)) {
        aiColor4D specular;
        FloatColor specular_f = FloatColor(0.0f, 0.0f, 0.0f);
        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_COLOR_SPECULAR, specular)) {
            specular_f = FloatColor((float)specular.r, (float)specular.g, (float)specular.b);
        }
        metalic = specular_f.max_component();
    }
    material.metallic = metalic;

    float transmission = 0.0f;
    if (AI_SUCCESS != assimp_material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission)) {
        if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_OPACITY, transmission)) {
            transmission = 1.0f - transmission;
        }
    }
    material.transmission = transmission;

    if (AI_SUCCESS == assimp_material->Get(AI_MATKEY_REFRACTI, float_dump)) {
        material.ior = float_dump;
    }
}

void Mesh::dump_from_assimp_material_to_internal_material(aiMaterial* assimp_material)
{
    read_mtl_material(material, assimp_material);
    read_pbr_material(pbr_material, assimp_material);

    pbr_material.print();
}