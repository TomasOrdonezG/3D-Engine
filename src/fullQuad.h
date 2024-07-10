#ifndef FULL_QUAD_H
#define FULL_QUAD_H

#include <glad/glad.h>
#include "shader.h"

class FullQuad
{
public:

    GLuint VAO, VBO;

    FullQuad() {}

    void init()
    {
        shader = Shader("./src/shaders/quad.vert", "./src/shaders/quad.frag");

        // Vertex data for a full-screen quad
        float quadVertices[] = {
            // Positions   // Texture Coords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
        };
        
        // Create one VAO and VBO 
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind VAO, this means next functions will affect `VAO`
        glBindVertexArray(VAO);

        // Similarily, bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Attach our data to the buffer `VBO`
        // We send the size as well as GL_STATIC_DRAW since we won't be sending it more than once
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // Specify the attributes
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GL_FLOAT), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GL_FLOAT), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);

        // Finally, unbind VBO and VAO after we're done with them
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    void useShader()
    {
        shader.use();
    }

    void render()
    {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

private:

    Shader shader;

};

#endif