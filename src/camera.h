#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include "shader.h"
#include "sphere.h"

class Camera
{
public:

    // States
    bool didUpdateThisFrame = true;

    // Attributes
    glm::vec3 lookfrom;
    glm::vec3 lookat = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 u, v, w;                                      // Basis vectors for the camera frame
    float distance = 5.0, theta = 0.0, phi = PI / 2.0f;     // Spherical coordinates for third person mode
    float focalLength = 3.0;

    // Viewport
    struct Viewport {
        float width;
        float height = 2.0;
        glm::vec3 pixelDH;
        glm::vec3 pixelDV;
        glm::vec3 pixelOrigin;
    } viewport;

    Camera() { onUpdate(); }

    void onUpdate()
    {
        didUpdateThisFrame = true;
    }

    void updateDimensions(float windowAspectRatio)
    {
        viewport.width = viewport.height * windowAspectRatio;
        onUpdate();
    }

    void setUniforms(Shader shader, const Window *window)
    {
        // Recalculate camera attributes if the camera moved since last frame
        if (didUpdateThisFrame)
        {
            lookfrom = lookat + glm::vec3(
                distance*cos(theta)*sin(phi),
                distance*cos(phi),
                distance*sin(theta)*sin(phi)
            );

            w = glm::normalize(lookfrom - lookat);
            u = glm::normalize(glm::cross(up, w));
            v = glm::cross(w, u);

            // Vectors across the horizontal and vertical viewport edges
            glm::vec3 viewportHorizontal = viewport.width * u;
            glm::vec3 viewportVertical = viewport.height * v;

            // Horizontal and vertical delta vectors from pixel to pixel
            viewport.pixelDH = viewportHorizontal / (float)window->width;
            viewport.pixelDV = viewportVertical / (float)window->height;

            // Location of the upper left pixel
            glm::vec3 viewportTopLeft = lookfrom - (focalLength*w) - (viewportHorizontal / 2.0f) - (viewportVertical / 2.0f);
            viewport.pixelOrigin = viewportTopLeft + 0.5f*(viewport.pixelDH + viewport.pixelDV);

            didUpdateThisFrame = false;
        }

        // ! TEMP: Set camera uniforms
        shader.setVec3f("lookfrom", lookfrom);

        // ! TEMP: Set viewport uniforms
        shader.setVec3f("pixelDH", viewport.pixelDH);
        shader.setVec3f("pixelDV", viewport.pixelDV);
        shader.setVec3f("pixelOrigin", viewport.pixelOrigin);

        // TODO: Create a camera UBO and a viewport UBO
        // glBindBuffer(GL_UNIFORM_BUFFER, uboData);
        // glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
        // glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void focusSphere(Sphere *sphere)
    {
        lookat = sphere->position;
        distance = sphere->radius * 10;
        onUpdate();
    }

private:


};

#endif