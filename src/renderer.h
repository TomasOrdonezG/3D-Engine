#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <stdlib.h>
#include <vector>
#include <imgui/imgui.h>
#include "utils.h"
#include "shader.h"
#include "window.h"
#include "material.h"
#include "fullQuad.h"
#include "sphere.h"
#include "camera.h"

class Renderer
{
public:

    Camera camera;

    // Renderer settings
    float u_time;
    int maxRayBounce = 50;
    int renderedFrameCount = 0;
    int samplesPerPixel = 1;
    int test = 0;
    int sky = 1;
    int doGammaCorrection = 1;
    int doTemporalAntiAliasing = 1;

    // States
    bool doTAA = true;

    Renderer () {}

    Renderer(float windowAspectRatio)
    {
        camera.updateDimensions(windowAspectRatio);

        // Compile and link shader programs
        rayTracingShader = Shader("./src/shaders/RayTracing.vert", "./src/shaders/RayTracing.frag");

        // // Create data UBO
        // glGenBuffers(1, &uboData);
        // glBindBuffer(GL_UNIFORM_BUFFER, uboData);
        // glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera::UniformData), NULL, GL_DYNAMIC_DRAW);
        // glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // // Bind the data UBO to a _bin point
        // GLuint blockIndex = glGetUniformBlockIndex(rayTracingShader.ID, "UniformBlock");
        // glUniformBlockBinding(rayTracingShader.ID, blockIndex, uboDataBindingPoint);
        // glBindBufferBase(GL_UNIFORM_BUFFER, uboDataBindingPoint, uboData);

        createWorld();
    }

    void onUpdate()
    {
        renderedFrameCount = 0;
        skipAA = 2;  // Skip anti aliasing for the next 2 frames
    }
    
    void renderScene(const Window *window, int prevTextureUnit, FullQuad *quad)
    {
        debugMenu();
        
        // Check if camera was updated
        if (camera.didUpdateThisFrame) onUpdate();
        // TODO: Check if scene was updated (Once scene is moved to another class)
        
        // Set uniforms
        camera.setUniforms(rayTracingShader, window);
        setSceneUniforms();
        setSettingsUniforms(prevTextureUnit);

        // Render scene
        rayTracingShader.use();
        quad->render();

        renderedFrameCount++;
    }

    void debugMenu()
    {
        static bool debug = false;
        if (ImGui::IsKeyPressed(ImGuiKey_H)) debug = !debug;
        
        #define DEBUG_VEC3(V) ImGui::Text(#V ": (%.4f, %.4f, %.4f)", V.x, V.y, V.z);

        if (debug)
        {
            ImGui::Text("Viewport Dimensions: (%.2f, %.2f)", camera.viewport.width, camera.viewport.height);
            ImGui::Text("Theta: %2.f, Phi: %.2f", camera.theta, camera.phi);
            DEBUG_VEC3(camera.position);
            DEBUG_VEC3(camera.viewport.pixelDH);
            DEBUG_VEC3(camera.viewport.pixelDV);
            DEBUG_VEC3(camera.viewport.pixelOrigin);
        }
    }

    void setSettingsUniforms(GLint prevTextureUnit)
    {
        // TODO: Add all these uniforms in a UBO
        doTemporalAntiAliasing = skipAA ? --skipAA > 1 : doTAA;
        rayTracingShader.setBool("doTemporalAntiAliasing", doTemporalAntiAliasing);
        rayTracingShader.setBool("doGammaCorrection", doGammaCorrection);
        rayTracingShader.setBool("test", test);
        rayTracingShader.setBool("sky", sky);

        u_time = (float)glfwGetTime() / 1000.0f;
        rayTracingShader.setFloat("u_time", u_time);

        rayTracingShader.setInt("maxRayBounce", maxRayBounce);
        rayTracingShader.setInt("renderedFrameCount", renderedFrameCount);
        rayTracingShader.setInt("samplesPerPixel", samplesPerPixel);
        rayTracingShader.setInt("previousFrame", prevTextureUnit);
    }

    void setSceneUniforms()
    {
        // Set sphere array uniform
        glBindBuffer(GL_UNIFORM_BUFFER, uboSpheres);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Sphere)*spheres.size(), spheres.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        rayTracingShader.setInt("spheresSize", spheres.size());
        rayTracingShader.setInt("selectedSphere", selectedSphere);
    }
    
    void selectSphere(glm::ivec2 windowCoord)
    {
        // Create ray from window coordinates
        glm::vec2 pos = glm::vec2(windowCoord.x, windowCoord.y);
        glm::vec3 pixelSample = camera.viewport.pixelOrigin + (pos.x * camera.viewport.pixelDH) + (pos.y * camera.viewport.pixelDV);
        glm::vec3 rayPos = camera.position;
        glm::vec3 rayDir = pixelSample - camera.position;

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

        if (!doesHit)
        {
            selectedSphere = -1;
            camera.selectSphere(NULL);
        }
        else
        {
            camera.selectSphere(&(spheres[selectedSphere]));
        }
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

    Sphere *getSelectedSphere()
    {
        return (selectedSphere != -1) ? &(spheres[selectedSphere]) : NULL;
    }

private:

    // Shader programs
    Shader rayTracingShader;
    // GLuint uboData, uboDataBindingPoint = 0;

    // Sphere array implementation
    // TODO: Implement `spheresSize`
    int spheresSize, maxSphereAmount = 20, selectedSphere = -1; // Index (-1) means no selected sphere
    std::vector<Sphere> spheres;
    GLuint uboSpheres, uboSpheresBindingPoint = 1;
    
    // States
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
        Dielectric orange(glm::vec3(0.5, 0.1, 0.0), 1.0, 1.0);
        Dielectric blue(glm::vec3(0.1, 0.95, 0.8), 1.0, 1.0);
        Mirror mirror(1.0);
        
        spheres.push_back(Sphere(orange, glm::vec3(0.0, 0.0, 0.0), 1.0));
        spheres.push_back(Sphere(blue, glm::vec3(0.0, -51, 0.0), 50.0));
    }

};

#endif