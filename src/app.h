#ifndef APP_H
#define APP_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include "debug.h"
#include "utils.h"
#include "shader.h"
#include "window.h"
#include "fullQuad.h"
#include "renderer.h"
#include "camera.h"
#include "sphere.h"
#include "material.h"

class App
{
public:

    App(int windowWidth, int windowHeight)
    {
        // GLFW
        glfwSetErrorCallback(errorCallBack);
        glfwInit();

        // GLFW window hints and context creation
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Full-screen window
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
        window = glfwCreateWindow(mode->width, mode->height, "Ray Tracing", primaryMonitor, NULL);
        // window = glfwCreateWindow(600, 600, "Ray Tracing", NULL, NULL); // Original..
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);  // Enable vsync

        // Initialize GLAD
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        
        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Properties of the ImGui window containing the OpenGL texture (that we draw on)
        sceneWindow = Window(windowWidth, windowHeight);

        initImGui();
        quad.init();
        renderer = Renderer(sceneWindow.aspectRatio);
    }

    ~App()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void loop()
    {
        while (!glfwWindowShouldClose(window))
        {
            beginFrame();
            gui();

            ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                // Get window size and position
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                ImVec2 windowPos = ImGui::GetCursorScreenPos();

                pollEvents();

                // Get previous frame texture unit and bind it (this way we can use it in the scene shader)
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sceneWindow.textures[!pingpong]);

                // Bind and clear current frame buffer (this way anything we render gets rendered on this FBO's texture)
                glBindFramebuffer(GL_FRAMEBUFFER, sceneWindow.FBOs[pingpong]);
                glViewport(0, 0, sceneWindow.width, sceneWindow.height);
                glClear(GL_COLOR_BUFFER_BIT);
                
                // Render the scene
                renderer.renderScene(&sceneWindow, 0, &quad);

                // Unbind current FBO and previous texture
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                
                // Display current texture on ImGui window
                ImGui::ImageButton((void*)sceneWindow.textures[pingpong], ImVec2(sceneWindow.width, sceneWindow.height), ImVec2(0, 1), ImVec2(1, 0), 0);
                
                // Swap pingpong boolean for the next iteration
                pingpong = !pingpong;
            }
            ImGui::End();
            
            endFrame();
        }
    }

private:

    GLFWwindow *window;
    FullQuad quad;
    Window sceneWindow;
    Renderer renderer;
    bool pingpong = false;


    // * GUI

    void gui()
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        
        ImGui::Begin("Data");
        dataGui();
        ImGui::End();

        ImGui::Begin("Camera");
        cameraMenu();
        ImGui::End();

        ImGui::Begin("Controls");
        controlsMenu();
        ImGui::End();

        // Menus for selected spheres
        Sphere *sphere;
        if (renderer.isSphereSelected(&sphere))
        {
            ImGui::Begin("Sphere");
            sphereMenu(sphere);
            ImGui::End();

            ImGui::Begin("Material");
            materialMenu(&(sphere->material));
            ImGui::End();
        }
    }

    void dataGui()
    {
        ImGui::Text("%20s: %-10.4f", "FPS", ImGui::GetIO().Framerate);
        ImGui::Text("%20s: %-10d", "Frames sampled", renderer.renderedFrameCount);
    }

    void cameraMenu()
    {
        bool updated = false;
        Camera *camera = &(renderer.camera);

        updated |= ImGui::SliderFloat("Focal Length", &(camera->focalLength), 0.1, 10.0);
        updated |= ImGui::SliderFloat("Theta", &(camera->theta), 0.0, (float)2*PI);
        updated |= ImGui::SliderFloat("Phi", &(camera->phi), 0.0, (float)PI);
        
        if (updated) camera->onUpdate();
    }

    void controlsMenu()
    {
        bool updated = false;

        updated |= ImGui::Checkbox("Test", (bool*)&(renderer.test));
        updated |= ImGui::Checkbox("Sky", (bool*)&(renderer.sky));
        updated |= ImGui::Checkbox("Gamma Correct", (bool*)&(renderer.doGammaCorrection));
        updated |= ImGui::Checkbox("Temporal Anti-Aliasing", &(renderer.doTAA));

        updated |= ImGui::SliderInt("Max Tracing Depth", &(renderer.maxRayBounce), 1, 100);
        updated |= ImGui::SliderInt("Samples per pixel", &(renderer.samplesPerPixel), 1, 20);

        if (updated) renderer.onUpdate();
    }

    void sphereMenu(Sphere *sphere)
    {
        bool updated = false;

        updated |= ImGui::DragFloat3("Position", &(sphere->position[0]), 0.1);
        updated |= ImGui::DragFloat("Radius", &(sphere->radius), 0.1, 0.1, 100.0);

        if (ImGui::Button("Focus"))
        {
            updated = true;
            renderer.camera.focusSphere(renderer.getSelectedSphere());
        }

        if (updated) renderer.onUpdate();
    }

    void materialMenu(Material *mat)
    {
        bool updated = false;

        updated |= ImGui::ColorEdit3("Albedo", &(mat->albedo[0]));
        updated |= ImGui::SliderFloat("Roughness", &(mat->roughness), 0.0, 1.0);
        updated |= ImGui::SliderFloat("Reflectivity", &(mat->reflectivity), 0.0, 1.0);

        ImGui::SeparatorText("Light emission");

        updated |= ImGui::ColorEdit3("Emission Colour", &(mat->emissionColour[0]));
        updated |= ImGui::SliderFloat("Emission Strength", &(mat->emissionStrength), 0.0, 100.0);

        if (updated) renderer.camera.onUpdate();
    }


    // * General app methods

    void beginFrame()
    {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    
    void endFrame()
    {
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        
        glfwSwapBuffers(window);
    }

    void pollEvents()
    {
        static bool isMouseDragging = false;
        static ImVec2 lastMousePos;
        
        // Mouse and window attributes
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImVec2 windowPos = ImGui::GetCursorScreenPos();
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 mousePosRelative(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
        
        // Resize Window, textures, etc
        bool windowChangedSize = (int)windowSize.x != sceneWindow.width || (int)windowSize.y != sceneWindow.height;
        if (windowChangedSize)
        {
            sceneWindow.updateDimensions((int)windowSize.x, (int)windowSize.y);
            renderer.camera.updateDimensions(sceneWindow.aspectRatio);
        }

        // Check if cursor is inside window
        bool mouseInsideWindow = mousePosRelative.x >= 0 && mousePosRelative.x <= windowSize.x && mousePosRelative.y >= 0 && mousePosRelative.y <= windowSize.y;
        if (!mouseInsideWindow || !ImGui::IsWindowFocused())
        {
            isMouseDragging = false;
            return;
        }
        
        // Pan camera on mouse drag
        if (isMouseDragging)
        {
            // Calculate change in mouse position
            ImVec2 dpos(mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y);

            // Calculate change in angle
            float dtheta = ((float)dpos.x / sceneWindow.width) * (2 * PI) * 0.8;
            float dphi = ((float)dpos.y / sceneWindow.height) * (2 * PI) * 0.2;

            // Update camera position
            renderer.camera.theta += dtheta;
            if (renderer.camera.theta <= 0.0) renderer.camera.theta = 2*PI;
            else if (renderer.camera.theta > 2*PI) renderer.camera.theta = 0.0;

            if (renderer.camera.phi >= 0.0 && renderer.camera.phi < PI) renderer.camera.phi -= dphi;

            renderer.camera.onUpdate();
        }
        
        // Mouse down and release events
        bool leftMouseClick = false;
        ImVec2 lastClickedPos = ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left];
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            isMouseDragging = mousePos.x != lastClickedPos.x || mousePos.y != lastClickedPos.y;
            lastMousePos = mousePos;
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            isMouseDragging = false;
            leftMouseClick = lastClickedPos.x == mousePos.x && lastClickedPos.y == mousePos.y;
        }
        
        // Mouse click events
        if (leftMouseClick)
        {
            renderer.selectSphere(glm::ivec2((int)mousePosRelative.x, (int)(windowSize.y - mousePosRelative.y)));
        }

        // Renderer zoom in (change in focal length)
        float yOffset = -ImGui::GetIO().MouseWheel;
        if (yOffset)
        {
            Sphere *selected = renderer.getSelectedSphere();
            float scale = (selected != NULL) ? selected->radius / 5.0 : 0.1;
            renderer.camera.distance += yOffset * scale;
            if (renderer.camera.distance < 0.1) renderer.camera.distance = 0.1;
            renderer.camera.onUpdate();
        }

    }

    void initImGui()
    {
        // Context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 

        // Style
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Renderer backend
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

};

#endif