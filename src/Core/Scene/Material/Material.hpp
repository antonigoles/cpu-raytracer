#pragma once
#include <Core/Color/Color.hpp>

class Material 
{
public:
    FloatColor diffuse = Color(0,0,0,0).as_floats();
    FloatColor specular = Color(0,0,0,0).as_floats();
    FloatColor ambient = Color(0,0,0,0).as_floats();
    FloatColor emission = Color(0,0,0,0).as_floats();
    float shininess = 1024;
    float opacity = 0; 
    int illumination = 0;
    bool is_emissive = false;

    aiShadingMode assimp_shading_mode;
};