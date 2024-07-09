#version 330 core

in vec2 TexCoords;
out vec4 FragColour;

uniform vec3 spherePos;
uniform float sphereRadius;
uniform float focalLength;
uniform vec3 cameraPos;
uniform vec2 viewportSize;
uniform ivec2 windowSize;

void main()
{
    // Draws a nice outline around a centered sphere

    vec4 outlineColour = vec4(1.0, 1.0, 0.0, 1.0);

    // Similar trianges say that: outlineDistance = radius*focalLength / dist(cameraPos, spherePos)
    float cameraDistance = distance(cameraPos, spherePos);
    float outlineDistance = sphereRadius*focalLength / cameraDistance;
    vec2 viewportCoord = gl_FragCoord.xy * (viewportSize / windowSize);
    bool outline = distance(viewportSize / 2.0, viewportCoord) <= outlineDistance;

    // Return outline colour if the fragment coords correspond to the sphere outline, transparent otherwise
    FragColour = outline ? outlineColour : vec4(0.0, 0.0, 0.0, 0.0);
}