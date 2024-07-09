#ifndef SPHERE_H
#define SPHERE_H

#include <glm/glm.hpp>
#include "material.h"

struct Sphere
{
    Material material = Dielectric(glm::vec3(0.5), 0.5, 0.5);  // sizeof(Material) = 16*3

    glm::vec3 position = glm::vec3(0.0);
    float radius = 1.0;
    
    Sphere() {}
    
    Sphere(Material material, glm::vec3 position, float radius)
        : position(position)
        , radius(radius)
        , material(material)
    {}

};

#endif