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
        camera = Camera(2.0f, sceneWindow.aspectRatio);
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

            ImGui::Begin("OpenGL Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
                camera.renderScene(&sceneWindow, 0, &quad);

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
    Camera camera;
    bool pingpong = false;

    void gui()
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        
        ImGui::Begin("Data");
        dataGui();
        ImGui::End();

        ImGui::Begin("Controls");
        controlsGui();
        ImGui::End();

        // Menus for selected spheres
        Sphere *sphere;
        if (camera.isSphereSelected(&sphere))
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
        ImGui::Text("%20s: %10.4f", "FPS", ImGui::GetIO().Framerate);
        ImGui::Text("%20s: %10d", "Frames sampled", camera.data.renderedFrameCount);
    }

    void controlsGui()
    {
        bool resetAA = false;
        bool cameraMoved = false;

        resetAA |= ImGui::Checkbox("Test", (bool*)&(camera.data.test));
        resetAA |= ImGui::Checkbox("Sky", (bool*)&(camera.data.sky));
        resetAA |= ImGui::Checkbox("Gamma Correct", (bool*)&(camera.data.doGammaCorrection));
        resetAA |= ImGui::Checkbox("Temporal Anti-Aliasing", &(camera.doTAA));

        resetAA |= ImGui::SliderInt("Max Tracing Depth", &(camera.data.maxRayBounce), 1, 100);
        resetAA |= ImGui::SliderInt("Samples per pixel", &(camera.data.samplesPerPixel), 1, 20);

        cameraMoved |= ImGui::SliderFloat("Focal Length", &(camera.focalLength), 0.1, 10.0);
        cameraMoved |= ImGui::SliderFloat("Theta", &(camera.theta), 0.0, (float)2*PI);
        cameraMoved |= ImGui::SliderFloat("Phi", &(camera.phi), 0.0, (float)PI);

        resetAA |= cameraMoved;
        if (resetAA) camera.resetAntiAliasing();
        if (cameraMoved) camera.wasMoved();
    }

    void sphereMenu(Sphere *sphere)
    {
        bool resetAA = false;

        resetAA |= ImGui::SliderFloat3("Position", &(sphere->position[0]), -10.0, 10.0);
        resetAA |= ImGui::SliderFloat("Radius", &(sphere->radius), 0.01, 10.0);

        if (resetAA) camera.resetAntiAliasing();
    }

    void materialMenu(Material *mat)
    {
        bool resetAA = false;

        resetAA |= ImGui::ColorEdit3("Albedo", &(mat->albedo[0]));
        resetAA |= ImGui::SliderFloat("Roughness", &(mat->roughness), 0.0, 1.0);
        resetAA |= ImGui::SliderFloat("Reflectivity", &(mat->reflectivity), 0.0, 1.0);

        ImGui::SeparatorText("Light emission");

        resetAA |= ImGui::ColorEdit3("Emission Colour", &(mat->emissionColour[0]));
        resetAA |= ImGui::SliderFloat("Emission Strength", &(mat->emissionStrength), 0.0, 100.0);

        if (resetAA) camera.resetAntiAliasing();
    }

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
            camera.updateDimensions(sceneWindow.aspectRatio);
            camera.resetAntiAliasing();
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
            camera.resetAntiAliasing();

            camera.theta += dtheta;
            if (camera.theta <= 0.0) camera.theta = 2*PI;
            else if (camera.theta > 2*PI) camera.theta = 0.0;

            if (camera.phi >= 0.0 && camera.phi < PI) camera.phi -= dphi;

            camera.wasMoved();
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
            camera.selectSphere(glm::ivec2((int)mousePosRelative.x, (int)(windowSize.y - mousePosRelative.y)));
        }

        // Camera zoom in (change in focal length)
        float yOffset = -ImGui::GetIO().MouseWheel;
        if (yOffset)
        {
            Sphere *selected = camera.getSelectedSphere();
            float scale = (selected != NULL) ? selected->radius / 5.0 : 0.1;
            camera.distance += yOffset * scale;
            if (camera.distance < 0.1) camera.distance = 0.1;
            camera.resetAntiAliasing();
            camera.wasMoved();
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