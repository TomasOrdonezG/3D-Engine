#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:

    GLuint ID;

    Shader() {}

    Shader(const char *vertexPath, const char *fragmentPath)
    {
        // Retrieve the vertex and fragment shader code from filepaths
        std::string vertexCode, fragmentCode;
        std::ifstream vShaderFile, fShaderFile;

        // Ensure ifstream objects can throw exceptions
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // Open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            // Read file contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // Close file handlers
            vShaderFile.close();
            fShaderFile.close();

            // Convert code into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();

        // Compile vertex shader
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkErrors(vertex, "COMPILATION", "VERTEX");

        // Compile fragment shader
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkErrors(fragment, "COMPILATION", "FRAGMENT");

        // Create shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkErrors(ID, "LINKING", "PROGRAM");

        // Cleanup
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }
    

    // * FLOAT * //

    void setFloat(const std::string &name, GLfloat value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setVec2f(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    
    void setVec2f(const std::string &name, GLfloat x, GLfloat y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3f(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3f(const std::string &name, GLfloat x, GLfloat y, GLfloat z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4f(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4f(const std::string &name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    

    // * DOUBLE * //

    void setDouble(const std::string &name, GLdouble value) const
    {
        glUniform1d(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setVec2d(const std::string &name, const glm::dvec2 &value) const
    {
        glUniform2dv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    
    void setVec2d(const std::string &name, GLdouble x, GLdouble y) const
    {
        glUniform2d(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3d(const std::string &name, const glm::dvec3 &value) const
    {
        glUniform3dv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3d(const std::string &name, GLdouble x, GLdouble y, GLdouble z) const
    {
        glUniform3d(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4d(const std::string &name, const glm::dvec4 &value) const
    {
        glUniform4dv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4d(const std::string &name, GLdouble x, GLdouble y, GLdouble z, GLdouble w) const
    {
        glUniform4d(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }


    // * INT * //
    
    void setInt(const std::string &name, GLint value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setVec2i(const std::string &name, const glm::ivec2 &value) const
    {
        glUniform2iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    
    void setVec2i(const std::string &name, GLint x, GLint y) const
    {
        glUniform2i(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3i(const std::string &name, const glm::ivec3 &value) const
    {
        glUniform3iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3i(const std::string &name, GLint x, GLint y, GLint z) const
    {
        glUniform3i(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4i(const std::string &name, const glm::ivec4 &value) const
    {
        glUniform4iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4i(const std::string &name, GLint x, GLint y, GLint z, GLint w) const
    {
        glUniform4i(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }


    // * BOOL * //
    
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (GLint)value);
    }

private:

    void checkErrors(GLuint shader, const char *type, const char *name)
    {
        GLint success;
        char infoLog[512];

        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "ERROR::" << name << "_SHADER::" << type << "_FAILED\n" << infoLog << std::endl;
        }
    }

};

#endif