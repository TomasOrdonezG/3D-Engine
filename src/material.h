#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct Material
{
public:

    glm::vec3 albedo;
    float roughness;

    glm::vec3 emissionColour;
    float emissionStrength;
    
    float reflectivity;
    float _pad[3];

    Material(glm::vec3 albedo, float roughness, glm::vec3 emissionColour, float emissionStrength, float reflectance)
        : albedo(albedo)
        , emissionColour(emissionColour)
        , reflectivity(reflectance)
        , emissionStrength(emissionStrength)
        , roughness(roughness)
    {}

};

struct Light : public Material
{
    public: Light(glm::vec3 colour, float strength)
        : Material(glm::vec3(0.0), 0.0, colour, strength, 0.0)
    {}
};

struct Dielectric : public Material
{
    public: Dielectric(glm::vec3 albedo, float roughness, float reflectance)
        : Material(albedo, roughness, glm::vec3(0.0), 0.0, reflectance)
    {}
};

struct Mirror : public Dielectric
{
    public: Mirror(float smoothness = 1.0)
        : Dielectric(glm::vec3(1.0), -smoothness + 1.0, smoothness)
    {}
};

#endif