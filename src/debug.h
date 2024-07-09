#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <iostream>

#ifdef DEBUG
    #define PRINT_INT(V) printf(#V " = (%d, %d)\n", V.x, V.y)
    #define PRINT_VEC2I(V) printf(#V " = (%d, %d)\n", V.x, V.y)
    #define PRINT_VEC3I(V) printf(#V " = (%d, %d, %d)\n", V.x, V.y, V.z)
    #define PRINT_VEC4I(V) printf(#V " = (%d, %d, %d, %d)\n", V.x, V.y, V.z, V.w)

    #define PRINT_FLOAT(V) printf(#V " = (%.4f, %.4f)\n", V.x, V.y)
    #define PRINT_VEC2F(V) printf(#V " = (%.4f, %.4f)\n", V.x, V.y)
    #define PRINT_VEC3F(V) printf(#V " = (%.4f, %.4f, %.4f)\n", V.x, V.y, V.z)
    #define PRINT_VEC4F(V) printf(#V " = (%.4f, %.4f, %.4f, %.4f)\n", V.x, V.y, V.z, V.w)

    #define PRINT_DOUBLE(V) PRINT_FLOAT(V)
    #define PRINT_VEC2D(V) PRINT_VEC2F(V)
    #define PRINT_VEC3D(V) PRINT_VEC3F(V)
    #define PRINT_VEC4D(V) PRINT_VEC4F(V)
#else
    #define PRINT_INT(V)
    #define PRINT_VEC2I(V)
    #define PRINT_VEC3I(V)
    #define PRINT_VEC4I(V)

    #define PRINT_FLOAT(V)
    #define PRINT_VEC2F(V)
    #define PRINT_VEC3F(V)
    #define PRINT_VEC4F(V)

    #define PRINT_DOUBLE(V)
    #define PRINT_VEC2D(V)
    #define PRINT_VEC3D(V)
    #define PRINT_VEC4D(V)
#endif

void errorCallBack(int error, const char *description)
{
    std::cerr << "Error: " << description << std::endl;
}


#endif