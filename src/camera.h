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
    enum CameraMode { THIRD_PERSON = 0, FIRST_PERSON = 1 } cameraMode = FIRST_PERSON;

    // Attributes
    glm::vec3 position = glm::vec3(-2.0, 4.4, 8.2);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 u, v, w;                                      // Basis vectors for the camera frame
    float focalLength = 3.0;
    float theta = 2.0, phi = 1.2;                           // Panning angles for first person mode, or spherical coordinates for third person mode
    float distance = 10.0;                                   // Last sperical coordinate for third person mode (is calculated even if on first person mode)

    // Third person camera attributes
    glm::vec3 lookat = glm::vec3(0.0, 1.0, 0.0);
    Sphere *selectedSphere = NULL;

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
            if (cameraMode == THIRD_PERSON)
            {
                position = lookat + glm::vec3(
                    distance*cos(theta)*sin(phi),
                    distance*cos(phi),
                    distance*sin(theta)*sin(phi)
                );
                w = glm::normalize(position - lookat);
            }
            else if (cameraMode == FIRST_PERSON)
            {
                w = glm::vec3(cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi));
                distance = glm::distance(lookat, position);
            }

            u = glm::normalize(glm::cross(up, w));
            v = glm::cross(w, u);

            // Vectors across the horizontal and vertical viewport edges
            glm::vec3 viewportHorizontal = viewport.width * u;
            glm::vec3 viewportVertical = viewport.height * v;

            // Horizontal and vertical delta vectors from pixel to pixel
            viewport.pixelDH = viewportHorizontal / (float)window->width;
            viewport.pixelDV = viewportVertical / (float)window->height;

            // Location of the upper left pixel
            glm::vec3 viewportTopLeft = position - (focalLength*w) - (viewportHorizontal / 2.0f) - (viewportVertical / 2.0f);
            viewport.pixelOrigin = viewportTopLeft + 0.5f*(viewport.pixelDH + viewport.pixelDV);

            didUpdateThisFrame = false;
        }

        // ! TEMP: Set camera uniforms
        shader.setVec3f("lookfrom", position);

        // ! TEMP: Set viewport uniforms
        shader.setVec3f("pixelDH", viewport.pixelDH);
        shader.setVec3f("pixelDV", viewport.pixelDV);
        shader.setVec3f("pixelOrigin", viewport.pixelOrigin);

        // TODO: Create a camera UBO and a viewport UBO
        // glBindBuffer(GL_UNIFORM_BUFFER, uboData);
        // glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
        // glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void selectSphere(Sphere *sphere)
    {
        selectedSphere = sphere;
    }

    void focusSphere(Sphere *sphere)
    {
        lookat = sphere->position;
        distance = sphere->radius * 10;
        onUpdate();
    }

    void settingsGUI()
    {
        bool updated = false;

        updated |= ImGui::SliderInt("Camera Mode", (int*)(&cameraMode), 0, 1, (cameraMode == FIRST_PERSON) ? "First person" : "Third person");
        updated |= ImGui::DragFloat("Focal Length", &focalLength, 0.1);
        updated |= ImGui::SliderFloat("Theta", &theta, 0.0, (float)2*PI);
        updated |= ImGui::SliderFloat("Phi", &phi, 0.0, (float)PI);

        if (cameraMode == Camera::FIRST_PERSON)
        {
            updated |= ImGui::DragFloat3("Position", &(position[0]), 0.1);
        }
        else if (cameraMode == Camera::THIRD_PERSON)
        {
            updated |= ImGui::DragFloat("Distance", &distance, 0.1);
        }
        
        if (updated) onUpdate();
    }

    void events(Window *window, ImVec2 lastMousePos, bool isMouseDragging)
    {
        // Mouse and window attributes
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImVec2 windowPos = ImGui::GetCursorScreenPos();
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 mousePosRelative(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
        
        // Pan camera on mouse drag
        if (isMouseDragging)
        {
            // Calculate change in mouse position
            ImVec2 dpos(mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y);

            // Calculate change in angle
            float dtheta = ((float)dpos.x / window->width) * (2 * PI) * 0.8;
            float dphi = ((float)dpos.y / window->height) * (2 * PI) * 0.2;

            // Update camera position
            theta += dtheta;
            if (theta <= 0.0) theta = 2*PI;
            else if (theta > 2*PI) theta = 0.0;

            phi -= dphi;
            if (phi < 0.1) phi = 0.1;       // Shouldn't get to `phi = 0`
            else if (phi > PI) phi = PI;

            onUpdate();
        }

        // Renderer zoom in (change in focal length)
        float yOffset = -ImGui::GetIO().MouseWheel;
        if (yOffset && cameraMode != FIRST_PERSON)
        {
            float scale = (selectedSphere != NULL) ? selectedSphere->radius / 5.0 : 0.1;
            distance += yOffset * scale;
            if (distance < 0.1) distance = 0.1;
            onUpdate();
        }

        // Move camera if its in first person using WASD, LCTRL, and SPACE keys for movement in 3D space
        if (cameraMode == FIRST_PERSON)
        {
            float offset = 0.05;

            if (ImGui::IsKeyDown(ImGuiKey_W))
            {
                position -= w*offset;
                onUpdate();
            }
            if (ImGui::IsKeyDown(ImGuiKey_A))
            {
                position -= u*offset;
                onUpdate();
            }
            if (ImGui::IsKeyDown(ImGuiKey_S))
            {
                position += w*offset;
                onUpdate();
            }
            if (ImGui::IsKeyDown(ImGuiKey_D))
            {
                position += u*offset;
                onUpdate();
            }
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
            {
                position -= v*offset;
                onUpdate();
            }
            if (ImGui::IsKeyDown(ImGuiKey_Space))
            {
                position += v*offset;
                onUpdate();
            }
        }
    }

};

#endif