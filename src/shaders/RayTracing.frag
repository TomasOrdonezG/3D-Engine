#version 330 core

// * Inputs / Outputs
in vec2 TexCoords;
out vec4 FragColour;

// * Macrodefinitions
#define FLOAT_MAX 3.402823466e+38
#define FLOAT_MIN 1.175494351e-38
#define PI 3.14159265358979323846
#define MAX_SPHERE_AMOUNT 20

// * Struct definitions
struct Ray { vec3 position, direction; };
struct Material { vec3 albedo; float roughness; vec3 emissionColour; float emissionStrength; float reflectivity; };
struct Sphere { Material material; vec3 position; float radius; };
struct RayHit { vec3 normal; float t; vec3 intersection; bool selected; Material material; };

// * Uniforms

// TODO: Camera UBO
uniform vec3 lookfrom;

// TODO: Viewport UBO
uniform vec3 pixelDH;
uniform vec3 pixelDV;
uniform vec3 pixelOrigin;

// TODO: Renderer settings UBO
uniform bool doTemporalAntiAliasing;
uniform bool doPixelSampling;
uniform int samplingMethod;
uniform bool doGammaCorrection;
uniform bool test;
uniform bool sky;
uniform float u_time;
uniform int maxRayBounce;
uniform int renderedFrameCount;
uniform int samplesPerPixel;
uniform sampler2D previousFrame;

// Scene UBO (only an array of spheres for now)
layout(std140) uniform Spheres { Sphere spheres[MAX_SPHERE_AMOUNT]; };
uniform int spheresSize;
uniform int selectedSphere;

// * Spheres
bool hitSphere(Sphere sphere, Ray ray, out RayHit hit)
{
    vec3 oc = sphere.position - ray.position;
    float a = dot(ray.direction, ray.direction);
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - sphere.radius*sphere.radius;

    float discriminant = h*h - a*c;
    if (discriminant < 0) return false;

    float sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    float root = (h - sqrtd) / a;
    if (root <= 0.001 || FLOAT_MAX <= root)
    {
        root = (h + sqrtd) / a;
        if (root <= 0.001 || FLOAT_MAX <= root)
            return false;
    }

    hit.t = root;
    hit.intersection = ray.position + hit.t*ray.direction;
    hit.material = sphere.material;

    vec3 outwardNormal = (hit.intersection - sphere.position) / sphere.radius;  // Normalizes it
    bool frontFace = dot(ray.direction, outwardNormal) < 0;
    hit.normal = frontFace ? outwardNormal : -outwardNormal;

    return true;
}

// * Utility functions
float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}

float rand()
{
    return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)) * u_time) * 43758.5453);
}

vec2 boxMuller(vec2 u)
{
    float r = sqrt(-2.0 * log(u.x));
    float theta = 2.0 * PI * u.y;
    return r * vec2(cos(theta), sin(theta));
}

vec3 randGaussianVec()
{
    vec2 u1 = vec2(rand(gl_FragCoord.xy), rand(gl_FragCoord.yx));
    vec2 u2 = vec2(rand(gl_FragCoord.yx + 0.5), rand(gl_FragCoord.xy + 0.5));
    vec2 gauss1 = boxMuller(u1);
    vec2 gauss2 = boxMuller(u2);
    return vec3(gauss1.x, gauss1.y, gauss2.x);
}

vec3 randGaussianUnitVec()
{
    return normalize(randGaussianVec());
}

vec3 randInHemisphere(vec3 normal)
{
    vec3 randomDir = randGaussianUnitVec();
    return (dot(randomDir, normal) > 0.0) ? randomDir : -randomDir;
}

vec3 gammaCorrect(vec3 linear)
{
    return vec3(sqrt(linear.x), sqrt(linear.y), sqrt(linear.z));
}

vec3 postProcess(vec3 colour)
{

    if (doGammaCorrection)
    {
        colour = gammaCorrect(colour);
    }
    
    if (doTemporalAntiAliasing)
    {
        // Average colour with previous frame
        vec3 prevColour = texture(previousFrame, TexCoords).xyz;
        colour = mix(prevColour, colour, 1.0 / (renderedFrameCount + 1));
    }

    return colour;

}

// * Ray tracing
bool findClosestIntersection(Ray ray, out RayHit closestHit)
{
    bool doesHit = false;
    float lowest_t = FLOAT_MAX;
    
    for (int i = 0; i < spheresSize; i++)
    {
        RayHit hit;
        if (hitSphere(spheres[i], ray, hit))
        {
            doesHit = true;
            hit.selected = (selectedSphere == i);
            
            if (hit.t < lowest_t) {

                lowest_t = hit.t;
                closestHit = hit;
            }
        }
    }

    return doesHit;
}

vec3 missColour(Ray ray, vec3 rayColour)
{
    if (sky)
    {
        vec3 unitDirection = normalize(ray.direction);
        float alpha = 0.5*(2*unitDirection.y + 1.0);
        return ((1.0 - alpha)*vec3(1.0) + alpha*vec3(0.5, 0.7, 1.0))*rayColour;
    }
    else
    {
        return vec3(0.0)*rayColour;
    }
}

vec3 traceRay(Ray ray)
{
    vec3 incomingColour = vec3(0.0);
    vec3 rayColour = vec3(1.0);
    
    RayHit hit;
    
    for (int i = 0; i < maxRayBounce; i++)
    {
        if (!findClosestIntersection(ray, hit))
        {
            incomingColour += missColour(ray, rayColour);
            break;
        }

        // Accumulate light colour
        Material material = hit.material;
        vec3 emittedLight = material.emissionColour * material.emissionStrength;
        incomingColour += emittedLight * rayColour;
        rayColour *= material.albedo*material.reflectivity;

        // Bounce ray
        ray.position = hit.intersection + hit.normal*0.0001;
        vec3 perfectReflection = reflect(ray.direction, hit.normal);
        ray.direction = mix(perfectReflection, randInHemisphere(hit.normal), material.roughness);
    }

    return incomingColour;
}

vec3 calculateColour(vec2 coord)
{
    vec3 pixelSample = pixelOrigin + (coord.x * pixelDH) + (coord.y * pixelDV);
    Ray ray = Ray(lookfrom, pixelSample - lookfrom);
    return traceRay(ray);
}


// * Pixel sampling methods
vec3 randomPointSample()
{
    vec3 colour = vec3(0.0);
    vec2 pixelCenter = gl_FragCoord.xy + 0.5;

    for (int i = 0; i < samplesPerPixel; i++)
    {
        // Sample random window coords
        vec2 offset = vec2(rand() - 0.5, rand() - 0.5);
        vec2 sampledCoord = pixelCenter + offset;

        // Calculate colour
        colour += calculateColour(sampledCoord);
    }

    return colour / float(samplesPerPixel);
}

vec3 gridSample()
{
    vec3 colour = vec3(0.0);

    for (int i = 0; i < samplesPerPixel; i++)
    {
        for (int j = 0; j < samplesPerPixel; j++)
        {
            // Sample random window coords
            vec2 offset = (vec2(i, j) + 0.5) / float(samplesPerPixel);
            vec2 sampledCoord = gl_FragCoord.xy + offset;

            // Calculate colour
            colour += calculateColour(sampledCoord);
        }
    }

    return colour / float(samplesPerPixel*samplesPerPixel);
}

vec3 jitteredGridSample()
{
    vec3 colour = vec3(0.0);

    for (int i = 0; i < samplesPerPixel; i++)
    {
        for (int j = 0; j < samplesPerPixel; j++)
        {
            // Sample random window coords with jitter
            vec2 offset = (vec2(i, j) + vec2(rand(), rand())) / float(samplesPerPixel);
            vec2 sampledCoord = gl_FragCoord.xy + offset;

            // Calculate colour
            colour += calculateColour(sampledCoord);
        }
    }

    return colour / float(samplesPerPixel*samplesPerPixel);
}

// * Main

void main()
{

    vec3 currentColour;

    if (doTemporalAntiAliasing || doPixelSampling)
    {
        // Sample pixel based on some sampling method
        if (samplingMethod == 0)
            currentColour = randomPointSample();
        else if (samplingMethod == 1)
            currentColour = jitteredGridSample();
        else if (samplingMethod == 2)
            currentColour = gridSample();
    }
    else
    {
        // No sampling, calculate colour at the pixel's center
        currentColour = calculateColour(gl_FragCoord.xy + 0.5);
    }

    currentColour = postProcess(currentColour);
    FragColour = vec4(currentColour, 1.0);
}
