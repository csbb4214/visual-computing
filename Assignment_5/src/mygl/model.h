#pragma once

#include "mesh.h"
#include "texture.h"

struct Material
{
    std::string name;

    Vector3D emission;
    Vector3D ambient;
    Vector3D diffuse;
    Vector3D specular;
    float shininess;

    Texture map_emission;
    Texture map_ambient;
    Texture map_diffuse;
    Texture map_specular;
    Texture map_shininess;
    Texture map_normal;

    unsigned int indexOffset;
    unsigned int indexCount;
};
void materialDelete(std::vector<Material>& materials);
void materialDelete(Material& material);

struct Model
{
    Mesh mesh;
    std::string name;
    std::vector<Material> material;
};

std::vector<Model> modelLoad(const std::string &filepath);
void modelDelete(std::vector<Model>& models);
void modelDelete(Model& model);
