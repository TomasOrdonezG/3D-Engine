#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <stdlib.h>
#include <vector>
#include "utils.h"
#include "shader.h"
#include "window.h"
#include "material.h"
#include "fullQuad.h"
#include "sphere.h"

class Camera
{
public:

    // Camera attributes
    struct UniformData {
        glm::vec3 lookfrom; float _pad1;
        glm::vec3 pixelDH;  float _pad2;
        glm::vec3 pixelDV;  float _pad3;
        glm::vec3 pixelOrigin;

        float u_time;
        int maxRayBounce = 50;
        int renderedFrameCount = 0;
        int samplesPerPixel = 1;
        
        int test = 0;
        int sky = 1;
        int doGammaCorrection = 1;
        int doTemporalAntiAliasing = 1;
    } data;
    float distance = 5.0, theta = 0.0, phi = PI / 2.0f;   // Spherical coordinates of the camera with `lookfrom` as the center reference position
    float focalLength = 3.0;

    // States
    bool doTAA = true;

    Camera () {}

    Camera(float viewportHeight, float windowAspectRatio)
        : viewportHeight(viewportHeight)
    {
        updateDimensions(windowAspectRatio);

        // Compile and link shader programs
        rayTracingShader = Shader("C:/Users/samal/OneDrive/Desktop/Programs/C++/GLFW + GLAD setup/src/shaders/RayTracing.vert", "C:/Users/samal/OneDrive/Desktop/Programs/C++/GLFW + GLAD setup/src/shaders/RayTracing.frag");
        outlineShader = Shader("C:/Users/samal/OneDrive/Desktop/Programs/C++/GLFW + GLAD setup/src/shaders/RayTracing.vert", "C:/Users/samal/OneDrive/Desktop/Programs/C++/GLFW + GLAD setup/src/shaders/outline.frag");

        // Create data UBO
        glGenBuffers(1, &uboData);
        glBindBuffer(GL_UNIFORM_BUFFER, uboData);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera::UniformData), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Bind the data UBO to a _bin point
        GLuint blockIndex = glGetUniformBlockIndex(rayTracingShader.ID, "UniformBlock");
        glUniformBlockBinding(rayTracingShader.ID, blockIndex, uboDataBindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, uboDataBindingPoint, uboData);

        createWorld();
    }
    
    void renderScene(const Window *window, int prevTextureUnit, FullQuad *quad)
    {
        // Render scene
        rayTracingShader.use();
        setSceneUniforms(window, prevTextureUnit);
        quad->render();

        // Selection outlines
        if (selectedSphere != -1)
        {
            outlineShader.use();
            outlineShader.setVec3f("spherePos", spheres[selectedSphere].position);
            outlineShader.setFloat("sphereRadius", spheres[selectedSphere].radius);
            outlineShader.setVec3f("cameraPos", data.lookfrom);
            outlineShader.setFloat("focalLength", focalLength);
            outlineShader.setVec2f("viewportSize", viewportWidth, viewportHeight);
            outlineShader.setVec2f("windowSize", window->width, window->height);
            quad->render();
        }

        data.renderedFrameCount++;
    }
    
    void setSceneUniforms(const Window *window, int prevTextureUnit)
    {
        // Update camera attributes if the camera moved since last frame
        if (didCameraMove)
        {
            data.lookfrom = lookat + glm::vec3(
                distance*cos(theta)*sin(phi),
                distance*cos(phi),
                distance*sin(theta)*sin(phi)
            );

            w = glm::normalize(data.lookfrom - lookat);
            u = glm::normalize(glm::cross(up, w));
            v = glm::cross(w, u);

            // Vectors across the horizontal and vertical viewport edges
            glm::vec3 viewportHorizontal = viewportWidth * u;
            glm::vec3 viewportVertical = viewportHeight * v;

            // Horizontal and vertical delta vectors from pixel to pixel
            data.pixelDH = viewportHorizontal / (float)window->width;
            data.pixelDV = viewportVertical / (float)window->height;

            // Location of the upper left pixel
            glm::vec3 viewportTopLeft = data.lookfrom - (focalLength*w) - (viewportHorizontal / 2.0f) - (viewportVertical / 2.0f);
            data.pixelOrigin = viewportTopLeft + 0.5f*(data.pixelDH + data.pixelDV);

            didCameraMove = false;
        }

        // Set data uniforms
        data.doTemporalAntiAliasing = skipAA ? --skipAA > 1 : doTAA;
        data.u_time = (float)glfwGetTime() / 1000.0f;

        glBindBuffer(GL_UNIFORM_BUFFER, uboData);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Set sphere array uniform
        glBindBuffer(GL_UNIFORM_BUFFER, uboSpheres);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Sphere)*spheres.size(), spheres.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        rayTracingShader.setInt("spheresSize", spheres.size());
        rayTracingShader.setInt("previousFrame", prevTextureUnit);
        rayTracingShader.setInt("selectedSphere", selectedSphere);
    }
    
    void resetAntiAliasing()
    {
        // Will disable temporal anti aliasing for the next two frames (if it's turned on)
        // Call this function on every frame that will make the image move
        skipAA = 2;
        data.renderedFrameCount = 0;
    }
    
    void updateDimensions(float windowAspectRatio)
    {
        viewportWidth = viewportHeight * windowAspectRatio;
        wasMoved();
    }

    void selectSphere(glm::ivec2 windowCoord)
    {
        // Create ray from window coordinates
        glm::vec2 pos = glm::vec2(windowCoord.x, windowCoord.y);
        glm::vec3 pixelSample = data.pixelOrigin + (pos.x * data.pixelDH) + (pos.y * data.pixelDV);
        glm::vec3 rayPos = data.lookfrom;
        glm::vec3 rayDir = pixelSample - data.lookfrom;

        auto hitSphere = [rayDir, rayPos](Sphere sphere, float *t) -> bool
        {
            glm::vec3 oc = sphere.position - rayPos;
            float a = glm::dot(rayDir, rayDir);
            float h = glm::dot(rayDir, oc);
            float c = glm::dot(oc, oc) - sphere.radius*sphere.radius;

            float discriminant = h*h - a*c;
            if (discriminant < 0) return false;

            float sqrtd = sqrt(discriminant);

            // Find the nearest root that lies in the acceptable range.
            float root = (h - sqrtd) / a;
            if (root <= 0.001 || FLT_MAX <= root) {
            
                root = (h + sqrtd) / a;
                if (root <= 0.001 || FLT_MAX <= root)
                    return false;
            }

            *t = root;

            return true;
        };

        // Find closest intersection of the new ray (this will be the sphere that gets selected)
        bool doesHit = false;
        float lowest_t = FLT_MAX;
        for (int i = 0; i < spheres.size(); i++)
        {
            float t;
            if (hitSphere(spheres[i], &t))
            {
                doesHit = true;
                if (t < lowest_t)
                {
                    selectedSphere = i;
                    lowest_t = t;
                }
            }
        }

        // Center the selected sphere
        if (selectedSphere != -1 && lookat != spheres[selectedSphere].position)
        {
            lookat = spheres[selectedSphere].position;
            distance = spheres[selectedSphere].radius * 10;
            
            wasMoved();
            resetAntiAliasing();
        }

        if (!doesHit)
            selectedSphere = -1;
    }

    bool isSphereSelected(Sphere **sphere)
    {
        if (selectedSphere != -1)
        {
            *sphere = &spheres[selectedSphere];
            return true;
        }

        return false;
    }

    void wasMoved()
    {
        didCameraMove = true;
    }

    Sphere *getSelectedSphere()
    {
        return (selectedSphere != -1) ? &(spheres[selectedSphere]) : NULL;
    }

private:

    // Camera attributes
    glm::vec3 lookat = glm::vec3(0.0, 0.0, 0.0);    // The point that the camera is looking at
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);        // Upwards always is (0, 1, 0) in order to keep the image horizontal with the environment
    glm::vec3 u, v, w;  // Camera frame basis vectors

    // Viewport
    float viewportHeight, viewportWidth;

    // Shader programs
    Shader rayTracingShader, outlineShader;
    GLuint uboData, uboDataBindingPoint = 0;

    // Sphere array implementation
    // TODO: Implement `spheresSize`
    int spheresSize, maxSphereAmount = 20, selectedSphere = -1; // Index (-1) means no selected sphere
    std::vector<Sphere> spheres;
    GLuint uboSpheres, uboSpheresBindingPoint = 1;
    
    // States
    bool didCameraMove = true;
    int skipAA = 0;

    void createWorld()
    {
        // Create UBO
        glGenBuffers(1, &uboSpheres);
        glBindBuffer(GL_UNIFORM_BUFFER, uboSpheres);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Sphere)*maxSphereAmount, NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Bind the sphere UBO to a binding point
        GLuint blockIndex = glGetUniformBlockIndex(rayTracingShader.ID, "Spheres");
        glUniformBlockBinding(rayTracingShader.ID, blockIndex, uboSpheresBindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, uboSpheresBindingPoint, uboSpheres);

        // Create the world!
        Light light(glm::vec3(1.0), 100.0);
        Dielectric rough(glm::vec3(0.8, 0.0, 0.8), 1.0, 0.5);
        Dielectric ground(glm::vec3(1.0, 1.0, 1.0), 0.9, 0.5);
        Mirror mirror(1.0);
        
        spheres.push_back(Sphere(rough, glm::vec3(0.0, 0.0, 0.0), 1.0));
        // spheres.push_back(Sphere(mirror, glm::vec3(1.0, 0.0, 0.0), 0.5));
        // spheres.push_back(Sphere(ground, glm::vec3(0.0, -300.5, 0.0), 300.0));
        // spheres.push_back(Sphere(light, glm::vec3(1.0, 1.0, 120.0), 50.0));
    }

};

#endif