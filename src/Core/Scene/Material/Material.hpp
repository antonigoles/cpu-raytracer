#pragma once
#include "Core/FloatColor/FloatColor.hpp"
#include "Infrastructure/Logger/Logger.hpp"
#include <Core/Color/Color.hpp>
#include <assimp/material.h>

class Material 
{
public:
    FloatColor diffuse = Color(0,0,0,0).as_floats();
    FloatColor specular = Color(0,0,0,0).as_floats();
    FloatColor ambient = Color(0,0,0,0).as_floats();
    FloatColor emission = Color(0,0,0,0).as_floats();
    FloatColor transmission = Color(0,0,0,0).as_floats();
    float shininess = 1024;
    float opacity = 1.0f; 
    float refraction = 1.0f;
    int illumination = 0;
    bool is_emissive = false;

    aiShadingMode assimp_shading_mode;

    void print_material()
    {
        log_info("START MATERIAL");

        log_info("  diffuse: ", diffuse.red, " ", diffuse.green, " ", diffuse.blue);
        log_info("  specular: ", specular.red, " ", specular.green, " ", specular.blue);
        log_info("  ambient: ", ambient.red, " ", ambient.green, " ", ambient.blue);
        log_info("  emission: ", emission.red, " ", emission.green, " ", emission.blue);
        log_info("  transmission: ", transmission.red, " ", transmission.green, " ", transmission.blue);
        
        log_info("  shininess: ", shininess);
        log_info("  opacity: ", opacity);
        log_info("  refraction: ", refraction);

        log_info("END MATERIAL");
    }
};