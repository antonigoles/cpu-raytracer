#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include "Infrastructure/Logger/Logger.hpp"
#include "Misc/Math/Math.hpp"
#include <Core/Color/Color.hpp>
#include <assimp/material.h>
#include <glm/ext/quaternion_exponential.hpp>

class PBRMaterial 
{
public:
    FloatColor base_color = FloatColor(0.0f,0.0f,0.0f);
    float roughness = 0.5f;
    float metallic = 0.0f;
    float transmission = 0.0f;
    float ior = 1.5f;

    FloatColor emission = FloatColor(0.0f, 0.0f, 0.0f);

    bool is_emissive() const
    {
        return emission.strength() > 0.0f;
    }

    FloatColor calculate_R0() const {
        float f0_dielectric = glm::pow((ior - 1.0f) / (ior + 1.0f), 2.0f);
        FloatColor r0_dielectric(f0_dielectric, f0_dielectric, f0_dielectric);
        return Math::lerp_color(r0_dielectric, base_color, metallic);
    }

    bool is_rough() const {
        return roughness >= 0.001f;
    }

    bool is_visibly_rough() const {
        return roughness >= 0.3f;
    }

    aiShadingMode assimp_shading_mode;

    void print()
    {
        log_info("START PBR MATERIAL");

        log_info("  base_color: ", base_color.red, " ", base_color.green, " ", base_color.blue);
        log_info("  emission: ", emission.red, " ", emission.green, " ", emission.blue);
        log_info("  roughness: ", roughness);
        log_info("  metallic: ", metallic);
        log_info("  transmission: ", transmission);
        log_info("  ior: ", ior);

        log_info("END PBR MATERIAL");
    }
};