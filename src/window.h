#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
public:

    int width, height;
    double aspectRatio;
    GLuint textures[2], FBOs[2];

    Window () {}

    Window(int width, int height)
        : width(width)
        , height(height)
        , aspectRatio(width / double(height))
    { initFBOs(); }

    void updateDimensions(int newWidth, int newHeight)
    {
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
        aspectRatio = width / (float)height;

        // Update texture sizes
        for (int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:

    void initFBOs()
    {
        // Create FBOs and textures
        glGenFramebuffers(2, FBOs);
        glGenTextures(2, textures);

        for (int i = 0; i < 2; i++)
        {
            // Set texture parameters
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            // Attach textures to FBOs
            glBindFramebuffer(GL_FRAMEBUFFER, FBOs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Frame buffer not complete" << std::endl;
        }
        
        // Unbind texture and frame buffers
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

};

#endif